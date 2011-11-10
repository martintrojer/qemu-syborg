import qemu

class syborg_interrupt(qemu.devclass):
  REG_ID            = 0
  REG_STATUS        = 1
  REG_CURRENT       = 2
  REG_DISABLE_ALL   = 3
  REG_DISABLE       = 4
  REG_ENABLE        = 5
  REG_TOTAL         = 6

  def update(self):
    self.pending = self.level & self.enabled
    self.set_irq_level(0, len(self.pending) > 0)

  def set_input(self, n, level):
    if level:
      if n in self.level:
        return
      self.level.add(n)
    else:
      if n not in self.level:
        return
      self.level.discard(n)
    self.update()

  def create(self):
    self.num_inputs = self.properties["num-interrupts"]
    self.enabled = set()
    self.level = set()
    self.pending = set()
    self.create_interrupts(self.set_input, self.num_inputs)

  def read_reg(self, offset):
    offset >>= 2
    if offset == self.REG_ID:
      return 0xc51d0000
    elif offset == self.REG_STATUS:
      return len(self.pending)
    elif offset == self.REG_CURRENT:
      for i in range(self.num_inputs):
        if i in self.pending:
          return i
      return 0xffffffff
    elif offset == self.REG_TOTAL:
      return self.num_inputs
    return 0

  def write_reg(self, offset, value):
    offset >>= 2
    if offset == self.REG_DISABLE_ALL:
      self.enabled = set()
    elif offset == self.REG_DISABLE:
      if (value < self.num_inputs):
        self.enabled.discard(value)
    elif offset == self.REG_ENABLE:
      if (value < self.num_inputs):
        self.enabled.add(value)
    self.update()

  def save(self, f):
    f.put_u32(self.num_inputs)
    for i in range(self.num_inputs):
      val = 0
      if i in self.enabled:
        val |= 1
      if i in self.level:
        val |= 2
      f.put_u32(val)

  def load(self, f):
    val = f.get_u32()
    if val != self.num_inputs:
      raise ValueError, "Incorrect number of IRQs"
    self.level = set()
    self.enabled = set()
    for i in range(self.num_inputs):
      val = f.get_u32()
      if (val & 1) != 0:
        self.enabled.add(i)
      if (val & 2) != 0:
        self.level.add(i)
    self.update()

  # Device class properties
  regions = [qemu.ioregion(0x1000, readl=read_reg, writel=write_reg)]
  irqs = 1
  name = "syborg,interrupt"
  properties = {"num-interrupts":64}

qemu.register_device(syborg_interrupt)
