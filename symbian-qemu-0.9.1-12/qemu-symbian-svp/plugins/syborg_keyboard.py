import qemu

class syborg_keyboard(qemu.devclass):
  REG_ID           = 0
  REG_DATA         = 1
  REG_FIFO_COUNT   = 2
  REG_INT_ENABLE   = 3
  REG_FIFO_SIZE    = 4

  def update_irq(self):
    self.set_irq_level(0, (len(self.fifo) > 0) and self.int_enabled)

  def event(self, keycode):
    if (keycode == 0xe0) and not self.extension_bit:
      self.extension_bit = 0x80
      return
    val = (keycode & 0x7f) | self.extension_bit
    if (keycode & 0x80) != 0:
      val |= 0x80000000
    self.extension_bit = 0
    if len(self.fifo) < self.fifo_size:
      self.fifo.append(val)
    self.update_irq()

  def create(self):
    self.fifo_size = self.properties["fifo-size"]
    self.fifo=[]
    self.int_enabled = False
    self.extension_bit = 0
    qemu.register_keyboard(self.event)

  def read_reg(self, offset):
    offset >>= 2
    if offset == self.REG_ID:
      return 0xc51d0002
    elif offset == self.REG_DATA:
      if len(self.fifo) == 0:
        return 0xffffffff
      val = self.fifo.pop(0)
      self.update_irq();
      return val
    elif offset == self.REG_FIFO_COUNT:
      return len(self.fifo)
    elif offset == self.REG_INT_ENABLE:
      return self.int_enabled
    elif offset == self.REG_FIFO_SIZE:
      return self.fifo_size
    return 0

  def write_reg(self, offset, value):
    offset >>= 2
    if offset == self.REG_INT_ENABLE:
      self.int_enabled = ((value & 1) != 0)
      self.update_irq()

  def save(self, f):
    f.put_u32(self.fifo_size)
    f.put_u32(self.int_enabled)
    f.put_u32(self.extension_bit)
    f.put_u32(len(self.fifo))
    for x in self.fifo:
      f.put_u32(x)

  def load(self, f):
    if self.fifo_size != f.get_u32():
      raise ValueError, "fifo size mismatch"
    self.int_enabled = f.get_u32()
    self.extension_bit = f.get_u32()
    n = f.get_u32()
    self.fifo = []
    while n > 0:
      self.fifo.append(f.get_u32())
      n -= 1;

  # Device class properties
  regions = [qemu.ioregion(0x1000, readl=read_reg, writel=write_reg)]
  irqs = 1
  name = "syborg,keyboard"
  properties = {"fifo-size":16}

qemu.register_device(syborg_keyboard)
