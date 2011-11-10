import qemu

class syborg_timer(qemu.devclass):
  REG_ID            = 0
  REG_RUNNING       = 1
  REG_ONESHOT       = 2
  REG_LIMIT         = 3
  REG_VALUE         = 4
  REG_INT_ENABLE    = 5
  REG_INT_STATUS    = 6
  REG_FREQ          = 7

  def update(self):
    self.set_irq_level(0, (self.int_status & self.int_enable) != 0)

  def timer_tick(self):
    self.int_status |= 1
    if (self.oneshot):
      self.running = False
    self.update()

  def create(self):
    self.freq = self.properties["frequency"]
    if self.freq == 0:
      raise ValueError, "Zero/unset frequency"
    self.timer = qemu.ptimer(self.timer_tick, self.freq)
    self.running = False
    self.oneshot = False
    self.int_status = 0
    self.int_enable = 0
    self.limit = 0

  def read_reg(self, offset):
    offset >>= 2
    if offset == self.REG_ID:
      return 0xc51d0003
    elif offset == self.REG_RUNNING:
      return 1 if self.running else 0
    elif offset == self.REG_ONESHOT:
      return 1 if self.oneshot else 0
    elif offset == self.REG_LIMIT:
      return self.limit
    elif offset == self.REG_VALUE:
      return self.timer.count
    elif offset == self.REG_INT_ENABLE:
      return self.int_enable
    elif offset == self.REG_INT_STATUS:
      return self.int_status
    elif offset == self.REG_FREQ:
      return self.freq
    return 0

  def write_reg(self, offset, value):
    offset >>= 2
    if offset == self.REG_RUNNING:
      if self.running != (value != 0):
        self.running = (value != 0)
        if self.running:
          self.timer.run(self.oneshot)
        else:
          self.timer.stop()
    elif offset == self.REG_ONESHOT:
      if self.running:
        self.timer.stop()
      self.oneshot = (value != 0)
      if self.running:
        self.timer.run(self.oneshot);
    elif offset == self.REG_LIMIT:
      self.limit = value
      self.timer.set_limit(value, True)
    elif offset == self.REG_VALUE:
      self.timer.count = value
    elif offset == self.REG_INT_ENABLE:
      self.int_enable = value & 1
      self.update()
    elif offset == self.REG_INT_STATUS:
      self.int_status &= ~value
      self.update()

  def save(self, f):
    f.put_u32(self.running)
    f.put_u32(self.oneshot)
    f.put_u32(self.limit)
    f.put_u32(self.int_status)
    f.put_u32(self.int_enable)
    self.timer.put(f)

  def load(self, f):
    self.running = (f.get_u32() != 0)
    self.oneshot = (f.get_u32() != 0)
    self.limit = f.get_u32()
    self.int_status = f.get_u32()
    self.int_enable = f.get_u32()
    self.timer.get(f)

  # Device class properties
  regions = [qemu.ioregion(0x1000, readl=read_reg, writel=write_reg)]
  irqs = 1
  name = "syborg,timer"
  properties = {"frequency":0}

qemu.register_device(syborg_timer)
