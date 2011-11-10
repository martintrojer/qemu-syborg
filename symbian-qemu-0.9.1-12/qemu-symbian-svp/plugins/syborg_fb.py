import qemu

class syborg_fb(qemu.devclass):
  REG_ID            = 0
  REG_BASE          = 1
  REG_HEIGHT        = 2
  REG_WIDTH         = 3
  REG_ORIENTATION   = 4
  REG_BLANK         = 5
  REG_INT_MASK      = 6
  REG_INT_CAUSE     = 7
  REG_BPP           = 8
  REG_COLOR_ORDER   = 9
  REG_BYTE_ORDER    = 10
  REG_PIXEL_ORDER   = 11
  REG_ROW_PITCH     = 12
  REG_ENABLED       = 13
  REG_PALETTE       = 0x100

  INT_VSYNC         = (1 << 0)
  INT_BASE_UPDATE   = (1 << 1)

  def update_irq(self):
    self.set_irq_level(0, (self.int_status & self.int_enable) != 0)

  def do_update(self):
    old_status = self.int_status
    if self.need_int:
      self.int_status |= self.INT_BASE_UPDATE
      self.need_int = False
    if self.render.blank == 0:
      self.int_status |= self.INT_VSYNC
    if self.int_status != old_status:
      self.update_irq()
    return self.enabled

  def create(self):
    self.enabled = False
    self.int_status = 0
    self.int_enable = 0
    self.need_int = True

    width = self.properties["width"]
    height = self.properties["height"]
    self.render = qemu.render(self.name, width, height)
    self.render.update = self.do_update

  def read_reg(self, offset):
    offset >>= 2
    if offset == self.REG_ID:
      return 0xc51d0006
    elif offset == self.REG_BASE:
      return self.render.base;
    elif offset == self.REG_WIDTH:
      return self.render.width;
    elif offset == self.REG_HEIGHT:
      return self.render.height;
    elif offset == self.REG_ORIENTATION:
      return self.render.orientation;
    elif offset == self.REG_BLANK:
      return self.render.blank;
    elif offset == self.REG_INT_MASK:
      return self.int_enable;
    elif offset == self.REG_INT_CAUSE:
      return self.int_status;
    elif offset == self.REG_BPP:
      return self.render.bpp;
    elif offset == self.REG_COLOR_ORDER:
      return self.render.color_order;
    elif offset == self.REG_BYTE_ORDER:
      return self.render.byte_order;
    elif offset == self.REG_PIXEL_ORDER:
      return self.render.pixel_order;
    elif offset == self.REG_ROW_PITCH:
      return self.render.row_pitch;
    elif offset == self.REG_ENABLED:
      return 1 if self.enabled else 0;
    elif (offset >= self.REG_PALETTE) and (offset < self.REG_PALETTE + 256):
      return self.render.palette[offset - self.REG_PALETTE];
    return 0

  def write_reg(self, offset, value):
    offset >>= 2
    if offset == self.REG_BASE:
      self.render.base = value;
      self.need_int = True
    elif offset == self.REG_WIDTH:
      self.render.width = value;
    elif offset == self.REG_HEIGHT:
      self.render.height = value;
    elif offset == self.REG_ORIENTATION:
      self.render.orientation = value;
    elif offset == self.REG_BLANK:
      self.render.blank = value;
    elif offset == self.REG_INT_CAUSE:
      self.int_status &= ~value;
      self.update_irq()
    elif offset == self.REG_INT_MASK:
      self.int_enable = value & 3;
      self.update_irq()
    elif offset == self.REG_BPP:
      self.render.bpp = value;
    elif offset == self.REG_COLOR_ORDER:
      self.render.color_order = value;
    elif offset == self.REG_BYTE_ORDER:
      self.render.byte_order = value;
    elif offset == self.REG_PIXEL_ORDER:
      self.render.pixel_order = value;
    elif offset == self.REG_ROW_PITCH:
      self.render.row_pitch = value;
    elif offset == self.REG_ENABLED:
      self.enabled = value != 0;
    elif (offset >= self.REG_PALETTE) and (offset < self.REG_PALETTE + 256):
      self.render.palette[offset - self.REG_PALETTE] = value;

  def save(self, f):
    f.put_u32(1 if self.need_int else 0)
    f.put_u32(self.int_status)
    f.put_u32(self.int_enable)
    f.put_u32(1 if self.enabled else 0)
    self.render.put(f)

  def load(self, f):
    self.need_int = (f.get_u32() != 0)
    self.int_status = f.get_u32();
    self.int_enable = f.get_u32();
    self.enabled = (f.get_u32() != 0)
    self.render.get(f)

  # Device class properties
  regions = [qemu.ioregion(0x1000, readl=read_reg, writel=write_reg)]
  irqs = 1
  name = "syborg,framebuffer"
  properties = {"width":0, "height":0}

qemu.register_device(syborg_fb)
