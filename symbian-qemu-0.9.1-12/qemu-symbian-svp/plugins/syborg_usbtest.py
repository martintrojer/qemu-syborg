import qemu
import os

class syborg_usbtest(qemu.devclass):
  REG_ID           = 0
  REG_INT_ENABLE   = 1
  REG_DATA_TYPE    = 2
  REG_DMA_ADDR     = 3
  REG_DMA_SIZE     = 4

  def loadIMG(self):
    self.buf = open('test1.BMP','rb').read()
    self.bufC = open('test.BMP','rb').read()
    self.bmpsize = os.path.getsize('test1.BMP')
    self.Csize = os.path.getsize('test.BMP')

  def timertick(self):
    if self.cha==0:
        compSize=self.bmpsize
        buf=self.buf
        self.cha=1
    else:
        compSize=self.Csize
        buf=self.bufC
        self.cha=0
    if self.dma_size < compSize:
        self.dma_size = 0
    else:
        for x in buf:
	    ch = ord(x)
	    if self.dma_size > 0:
            	self.dma_writeb(self.dma_addr, ch)
            	self.dma_addr += 1
		self.dma_size -= 1
    self.set_irq_level(0, self.int_enable)

  def timer_on(self):
    self.ptimer = qemu.ptimer(self.timertick, 1)
    self.ptimer.run(0)

  def timer_off(self):
    self.ptimer.stop()
    self.set_irq_level(0, self.int_enable)

  def capturedata(self):
    if self.dma_size < self.Csize:
        self.dma_size = 0
    else:
        for x in self.bufC:
	    ch = ord(x)
	    if self.dma_size > 0:
            	self.dma_writeb(self.dma_addr, ch)
            	self.dma_addr += 1
		self.dma_size -= 1
    self.set_irq_level(0, self.int_enable)

  def create(self):
    self.int_enable = 1
    self.dma_addr = 0
    self.dma_size =0
    self.cha=0
    self.loadIMG()

  def write_reg(self, offset, value):
    offset >>= 2
    if offset==self.REG_INT_ENABLE:
        self.int_enable=value
        if value==1:
	    if self.data_type==0:
                self.timer_on()
            elif self.data_type==1:
                self.capturedata()
        else:
	    if self.data_type==0:
           	self.timer_off()
            elif self.data_type==1:
	        self.set_irq_level(0, self.int_enable)
    elif offset == self.REG_DATA_TYPE:
      self.data_type = value
    elif offset == self.REG_DMA_ADDR:
      self.dma_addr = value
    elif offset == self.REG_DMA_SIZE:
      self.dma_size = value

  def read_reg(self, offset):
    offset >>= 2
    if offset == self.REG_ID:
      return 0xc600f000
    elif offset == self.REG_INT_ENABLE:
      return self.int_enable
    elif offset == self.REG_DMA_ADDR:
      return self.dma_addr
    elif offset == self.REG_DMA_SIZE:
      return self.dma_size
    return 0

  def save(self, f):
    f.put_u32(self.int_enable)
    f.put_u32(self.dma_addr)
    f.put_u32(self.dma_size)

  def load(self, f):
    self.int_enable = f.get_u32()
    self.dma_addr = f.get_u32()
    self.dma_size = f.get_u32()

  # Device class properties
  regions = [qemu.ioregion(0x1000, readl=read_reg, writel=write_reg)]
  irqs = 1
  name = "syborg,usbtest"
  properties={"chardev":None}

qemu.register_device(syborg_usbtest)
