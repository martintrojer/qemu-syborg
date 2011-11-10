#
# Contributors:
# NTT DOCOMO, INC. -- Syborg QEMU crashes when using skin + touchscreen device
#

import qemu

class syborg_pointer(qemu.devclass):
  REG_ID           = 0
  REG_LATCH        = 1
  REG_FIFO_COUNT   = 2
  REG_X            = 3
  REG_Y            = 4
  REG_Z            = 5
  REG_BUTTONS      = 6
  REG_INT_ENABLE   = 7
  REG_FIFO_SIZE    = 8

  class fifo_entry:
    def __init__(self, x, y, z, buttons):
      self.x = x
      self.y = y
      self.z = z
      self.buttons = buttons

  def update_irq(self):
    self.set_irq_level(0, (len(self.fifo) > 0) and self.int_enabled)

  def event(self, x, y, z, buttons):
    if len(self.fifo) < self.fifo_size:
      self.fifo.append(self.fifo_entry(x, y, z, buttons))
    self.update_irq()

  def create(self):
    self.absolute = self.properties["absolute"]
    self.fifo_size = self.properties["fifo-size"]
    self.fifo=[]
    self.current = self.fifo_entry(0, 0, 0, 0)
    self.int_enabled = False
    qemu.register_mouse(self.event, self.absolute, self.name)

  def read_reg(self, offset):
    offset >>= 2
    if offset == self.REG_ID:
      return 0xc51d0006 if self.absolute else 0xc51d0005
    elif offset == self.REG_FIFO_COUNT:
      return len(self.fifo)
    elif offset == self.REG_X:
      return self.current.x
    elif offset == self.REG_Y:
      return self.current.y
    elif offset == self.REG_Z:
      return self.current.z
    elif offset == self.REG_BUTTONS:
      return self.current.buttons
    elif offset == self.REG_INT_ENABLE:
      return self.int_enabled
    elif offset == self.REG_FIFO_SIZE:
      return self.fifo_size
    return 0

  def write_reg(self, offset, value):
    offset >>= 2
    if offset == self.REG_LATCH:
      if len(self.fifo) == 0:
        return
      self.current = self.fifo.pop(0)
      self.update_irq()
    elif offset == self.REG_INT_ENABLE:
      self.int_enabled = ((value & 1) != 0)
      self.update_irq()

  def save(self, f):
    f.put_u32(self.fifo_size)
    f.put_u32(self.int_enabled)
    f.put_u32(len(self.fifo) + 1)
    for d in [self.current] + self.fifo:
      f.put_u32(d.x)
      f.put_u32(d.y)
      f.put_u32(d.z)
      f.put_u32(d.buttons)

  def load(self, f):
    if self.fifo_size != f.get_u32():
      raise ValueError, "fifo size mismatch"
    self.int_enabled = f.get_u32()
    n = f.get_u32()
    self.fifo = []
    while n > 0:
      x = f.get_u32()
      y = f.get_u32()
      z = f.get_u32()
      buttons = f.get_u32()
      self.fifo.append(self.fifo_entry(x, y, z, buttons))
      n -= 1;
    self.current = self.fifo.pop(0);

  # Device class properties
  regions = [qemu.ioregion(0x1000, readl=read_reg, writel=write_reg)]
  irqs = 1
  name = "syborg,pointer"
  properties = {"fifo-size":16, "absolute":1}

qemu.register_device(syborg_pointer)
