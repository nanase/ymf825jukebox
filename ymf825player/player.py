#!/usr/bin/env python3

import json
import threading
import time
from collections import namedtuple
from os import path, walk
from subprocess import PIPE, Popen

import RPi.GPIO as GPIO
from container import Container


def filepath(filename):
  return path.abspath(path.join(path.dirname(path.abspath(__file__)), filename))


class Player:
  def __init__(self, config=filepath('config.json')):
    with open(config) as file:
      config = json.load(file, object_hook=lambda d: namedtuple('X', d.keys())(*d.values()))
    
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(config.led_pins.power, GPIO.OUT)
    GPIO.setup(config.led_pins.playing, GPIO.OUT)
    GPIO.setup(config.led_pins.standby, GPIO.OUT)
    GPIO.setup(config.switch_pins.play, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(config.switch_pins.prev, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(config.switch_pins.next, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(config.switch_pins.directory, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(config.switch_pins.power, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    GPIO.add_event_detect(config.switch_pins.play, GPIO.FALLING, callback=self.on_play_switch, bouncetime=config.bouncetime)
    GPIO.add_event_detect(config.switch_pins.prev, GPIO.FALLING, callback=self.on_prev_switch, bouncetime=config.bouncetime)
    GPIO.add_event_detect(config.switch_pins.next, GPIO.FALLING, callback=self.on_next_switch, bouncetime=config.bouncetime)
    GPIO.add_event_detect(config.switch_pins.directory, GPIO.FALLING, callback=self.on_directory_switch, bouncetime=config.bouncetime)
    GPIO.add_event_detect(config.switch_pins.power, GPIO.FALLING, callback=self.on_power_switch, bouncetime=config.bouncetime)

    GPIO.output(config.led_pins.standby, True)
    GPIO.output(config.led_pins.playing, False)
    GPIO.output(config.led_pins.power, True)

    self.config = config
    self.playing = False
    self.playing_file = None
    self.reader_process = None
    self.base_dir = filepath(config.directory.music)
    self.file_index = 0
    self.directory_index = 0
    self.process_thread = None
    self.required_stop = False
    self.required_shutdown = False

  def __enter__(self):
    return self
  
  def __exit__(self, exc_type, exc_value, traceback):
    self.stop()
    GPIO.output(self.config.led_pins.standby, False)
    GPIO.cleanup()

  def play(self):
    if self.playing:
      return

    self.reader_process.send_signal(10)  # SIGUSER1
    print('resume')
    self.playing = True
    GPIO.output(self.config.led_pins.playing, True)
    
  def pause(self):
    if self.reader_process is None:
      return

    self.reader_process.send_signal(10)  # SIGUSER1
    print('suspend')
    self.playing = False
    GPIO.output(self.config.led_pins.playing, False)

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
      GPIO.output(self.config.led_pins.playing, False)

  def load(self):
    if self.reader_process is not None:
      return

    if self.process_thread is not None:
      return

    self.process_thread = threading.Thread(target=self.process)
    self.process_thread.start()

  def get_file_path(self):
    files = sorted([sorted([path.join(directory, f) for f in file_names if path.splitext(f)[1] == '.825'])
        for directory, _, file_names in walk(self.base_dir) if len(file_names) > 0])
    
    if len(files) == 0:
      return None
    elif self.directory_index < 0:
      self.directory_index = len(files) - 1
    elif self.directory_index >= len(files):
      self.directory_index = 0
    
    if len(files[self.directory_index]) == 0:
      return None
    elif self.file_index < 0:
      self.file_index = len(files[self.directory_index]) - 1
    elif self.file_index >= len(files[self.directory_index]):
      self.file_index = 0

    return files[self.directory_index][self.file_index]

  def seek_next_file(self):
    self.file_index += 1

  def seek_prev_file(self):
    self.file_index -= 1

  def seek_next_directory(self):
    self.file_index = 0
    self.directory_index += 1

  def process(self):
    while True:
      next_playing_file = self.get_file_path()

      print(next_playing_file)
      if next_playing_file is None:
        return

      self.playing = True
      self.playing_file = next_playing_file
      GPIO.output(self.config.led_pins.playing, True)

      container = Container(self.playing_file)

      self.reader_process = Popen([filepath(self.config.directory.reader), str(container.resolution)], stdin=PIPE)
      self.reader_process.communicate(container.dump)
      self.reader_process.wait()
      self.reader_process = None
      self.playing = False
      self.playing_file = None
      GPIO.output(self.config.led_pins.playing, False)

      if self.required_stop:
        break
      else:
        self.seek_next_file()
    
    print('end of process thread')

  def on_play_switch(self, gpio):
    if self.required_shutdown:
      return

    if self.reader_process is None:
      self.load()
      return

    if self.playing:
      self.pause()
    else:
      self.play()
  
  def on_prev_switch(self, gpio):
    if self.required_shutdown:
      return

    print('prev')
    self.stop()
    self.seek_prev_file()
    self.load()

  def on_next_switch(self, gpio):
    if self.required_shutdown:
      return

    print('next')
    self.stop()
    self.seek_next_file()
    self.load()

  def on_directory_switch(self, gpio):
    if self.required_shutdown:
      return
    
    print('directory')
    self.stop()
    self.seek_next_directory()
    self.load()

  def on_power_switch(self, gpio):
    if self.required_shutdown:
      return
    
    for _ in range(300):
      time.sleep(0.01)
      
      if GPIO.input(gpio) == GPIO.HIGH:
        return

    self.required_shutdown = True
    self.stop()
    print('shutdown!')

    for _ in range(5):
      GPIO.output(self.config.led_pins.power, False)
      time.sleep(0.25)
      GPIO.output(self.config.led_pins.power, True)
      time.sleep(0.25)
    
    time.sleep(1)
    from subprocess import call
    call("sudo poweroff", shell=True)


def main():
  with Player() as _:
    try:
      while True:
        time.sleep(1)
    except KeyboardInterrupt:
      pass


if __name__ == '__main__':
  main()
