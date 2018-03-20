import zipfile
import json


class Container:
  def __init__(self, container_file: str):
    with zipfile.ZipFile(container_file, 'r') as zip:
      self.extract_config(zip.read('config.json'))
      self.extract_dump(zip.read('dump'))

  def extract_config(self, bytes_config: bytes):
    config = json.loads(bytes_config.decode('utf-8'))

    if 'version' not in config or 'resolution' not in config:
      raise Container.BadContainerError()

    self._version = config['version']
    self._resolution = config['resolution']

    if self._version != 0:
      raise Container.BadContainerError()
    
    if not (0 < self._resolution <= 65536):
      raise Container.BadContainerError()

    if 'meta' in config:
      self._meta = config['meta']
    else:
      self._meta = None
  
  def extract_dump(self, bytes_dump: bytes):
    self._dump = bytes_dump

  @property
  def meta(self) -> dict:
    return self._meta

  @property
  def dump(self) -> bytes:
    return self._dump

  @property
  def version(self) -> int:
    return self._version
  
  @property
  def resolution(self) -> int:
    return self._resolution

  class BadContainerError(Exception):
    def __init__(self):
      pass
