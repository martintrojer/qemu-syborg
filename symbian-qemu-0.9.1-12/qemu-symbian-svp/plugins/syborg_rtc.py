import qemu

class syborg_timer(qemu.devclass):
  REG_ID            = 0
  REG_LATCH         = 1
  REG_DATA_LOW      = 2
  REG_DATA_HIGH     = 3

  def create(self):
    self.data = 0
    self.offset = qemu.start_time() * 1000000 - qemu.get_clock()

  def read_reg(self, offset):
    offset >>= 2
    if offset == self.REG_ID:
      return 0xc51d0004
    elif offset == self.REG_DATA_LOW:
      return self.data & 0xffffffff
    elif offset == self.REG_DATA_HIGH:
      return (self.data >> 32) & 0xffffffff
    return 0

  def write_reg(self, offset, value):
    offset >>= 2
    if offset == self.REG_LATCH:
      now = qemu.get_clock()
      if value >= 4:
        self.offset = self.data - now
      else:
        self.data = now + self.offset
        while value:
          self.data /= 1000
          value -= 1;
    elif offset == self.REG_DATA_LOW:
      self.data = (self.data & ~0xffffffff) | value
    elif offset == self.REG_DATA_HIGH:
      self.data = (self.data & 0xffffffff) | (value << 32)

  def save(self, f):
    f.put_s64(self.offset)
    f.put_u64(self.data)

  def load(self, f):
    self.offset = f.get_s64()
    self.data = f.get_u64()

  # Device class properties
  regions = [qemu.ioregion(0x1000, readl=read_reg, writel=write_reg)]
  irqs = 0
  name = "syborg,rtc"
  properties = {}

qemu.register_device(syborg_timer)
