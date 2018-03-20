#!/usr/bin/env python3

import json
import threading
import time
from os import path, listdir
from collections import namedtuple
from subprocess import PIPE, Popen

import RPi.GPIO as GPIO


class Player:
  def __init__(self, config='config.json'):
    with open('config.json') as file:
      config = json.load(file, object_hook=lambda d: namedtuple('X', d.keys())(*d.values()))
    
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(config.led_pins.power, GPIO.OUT)
    GPIO.setup(config.switch_pins.play, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(config.switch_pins.prev, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(config.switch_pins.next, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(config.switch_pins.directory, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    GPIO.add_event_detect(config.switch_pins.play, GPIO.FALLING, callback=self.on_play_switch, bouncetime=config.bouncetime)
    GPIO.add_event_detect(config.switch_pins.prev, GPIO.FALLING, callback=self.on_prev_switch, bouncetime=config.bouncetime)
    GPIO.add_event_detect(config.switch_pins.next, GPIO.FALLING, callback=self.on_next_switch, bouncetime=config.bouncetime)
    GPIO.add_event_detect(config.switch_pins.directory, GPIO.FALLING, callback=self.on_directory_switch, bouncetime=config.bouncetime)

    GPIO.output(config.led_pins.power, True)

    self.config = config
    self.playing = False
    self.playing_file = None
    self.reader_process = None

    self.file_index = -1
    self.directory_index = 0
    self.next_playing_file = None
    self.seek_next_file()

  def __enter__(self):
    return self
  
  def __exit__(self, exc_type, exc_value, traceback):
    self.stop()
    GPIO.cleanup()

  def play(self):
    if self.playing:
      return

    self.reader_process.send_signal(10)  # SIGUSER1
    print('resume')
    self.playing = True
    
  def pause(self):
    if self.reader_process is None:
      return

    self.reader_process.send_signal(10)  # SIGUSER1
    print('suspend')
    self.playing = False

  def stop(self):
    if self.reader_process is not None:
      self.required_stop = True
      
      self.reader_process.send_signal(2)  # SIGINT
      self.reader_process.wait()

      if self.process_thread is not None:
        self.process_thread.join()
        self.process_thread = None
      
      self.required_stop = False
      self.playing = False
      self.playing_file = None
      self.reader_process = None

  def load(self):
    if self.reader_process is not None:
      return

    process_thread = threading.Thread(target=self.process)
    process_thread.start()

  def seek_next_file(self):
    base_dir = self.config.directory.music
    directories = [f for f in listdir(base_dir) if path.isdir(path.join(base_dir, f))]
    
    if self.directory_index < 0 or self.directory_index >= len(directories):
      self.directory_index = 0
    
    if len(directories) == 0:
      self.next_playing_file = None
      return

    target_directory = path.join(base_dir, directories[self.directory_index])
    files = [f for f in listdir(target_directory) if path.isfile(path.join(target_directory, f)) and path.splitext(path.join(target_directory, f))[1] == '.825']
    self.file_index += 1

    if self.file_index < 0 or self.file_index >= len(files):
      self.file_index = 0

    if len(files) == 0:
      self.next_playing_file = None
    else:
      self.next_playing_file = path.join(target_directory, files[self.file_index])

  def seek_prev_file(self):
    base_dir = self.config.directory.music
    directories = [f for f in listdir(base_dir) if path.isdir(path.join(base_dir, f))]
    
    if self.directory_index < 0 or self.directory_index >= len(directories):
      self.directory_index = 0
    
    if len(directories) == 0:
      self.next_playing_file = None
      return

    target_directory = path.join(base_dir, directories[self.directory_index])
    files = [f for f in listdir(target_directory) if path.isfile(path.join(target_directory, f)) and path.splitext(path.join(target_directory, f))[1] == '.825']
    self.file_index -= 1

    if self.file_index < 0 or self.file_index >= len(files):
      self.file_index = len(files) - 1

    if len(files) == 0:
      self.next_playing_file = None
    else:
      self.next_playing_file = path.join(target_directory, files[self.file_index])

  def seek_next_directory(self):
    base_dir = self.config.directory.music
    directories = [f for f in listdir(base_dir) if path.isdir(path.join(base_dir, f))]
    self.directory_index += 1

    if self.directory_index < 0 or self.directory_index >= len(directories):
      self.directory_index = 0
      return
    
    if len(directories) == 0:
      self.next_playing_file = None
      return

    target_directory = path.join(base_dir, directories[self.directory_index])
    files = [f for f in listdir(target_directory) if path.isfile(path.join(target_directory, f)) and path.splitext(path.join(target_directory, f))[1] == '.825']
    self.file_index = 0

    if self.file_index < 0 or self.file_index >= len(files):
      self.file_index = 0

    if len(files) == 0:
      self.next_playing_file = None
    else:
      self.next_playing_file = path.join(target_directory, files[self.file_index])

  def process(self):
    print(self.next_playing_file)
    if self.next_playing_file is None:
      return

    self.playing = True
    self.playing_file = self.next_playing_file
    
    with open(self.playing_file, 'rb') as file:
      file.seek(0x10)
      dump = file.read()

    self.reader_process = Popen([self.config.directory.reader, '1000'], stdin=PIPE)
    self.reader_process.communicate(dump)
    self.reader_process.wait()
    self.reader_process = None
    self.playing = False
    self.playing_file = None

  def on_play_switch(self, gpio):
    if self.reader_process is None:
      self.load()
      return

    if self.playing:
      self.pause()
    else:
      self.play()
  
  def on_prev_switch(self, gpio):
    print('prev')
    self.stop()
    self.seek_prev_file()
    self.load()

  def on_next_switch(self, gpio):
    print('next')
    self.stop()
    self.seek_next_file()
    self.load()

  def on_directory_switch(self, gpio):
    print('directory')
    self.stop()
    self.seek_next_directory()
    self.load()


def main():
  with Player() as player:
    try:
      while True:
        time.sleep(0.1)
    except KeyboardInterrupt:
      pass


if __name__ == '__main__':
  main()
