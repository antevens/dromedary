# Velodrome marking/line sensor
import sensor, image, time, lcd, pyb, micropython




class VeloTimer():
    def __init__(self):
        # Configure the imaging sensor
        sensor.reset() # Initialize the sensor
        sensor.set_pixformat(sensor.GRAYSCALE) # Set pixel format
        sensor.set_framesize(sensor.QQQVGA)  # Set frame size
        sensor.set_auto_exposure(True, exposure_us=5000) # Smaller means faster
        sensor.skip_frames(time = 2000)     # Wait for settings take effect

        # Configure clock for tracking FPS
        self.clock = time.clock()

        # Configure the lcd screen.
        lcd.init()

        # Initialize image buffer
        self.img = sensor.snapshot()

        # Set line finding parameters
        self.min_degree = 45
        self.max_degree = 134
        self.threshold = 1000
        self.theta_margin = 25
        self.rho_margin = 25
        self.color = (255, 0, 0)

        # Schedule async LCD shield updates
        self.render_ref = self.render  # Allocation occurs here
        tim = pyb.Timer(4)
        tim.init(freq=20)
        tim.callback(self.cb)

    def cb(self, timer):
        """ Callback event handler for rendering to the LCD async """
        micropython.schedule(self.render_ref, timer)

    def snapshot(self):
        """ Takes a snapshot from the image sensor """
        self.img = sensor.snapshot()

    def render(self, timer, img=None):
        """ Renders an image to the LCD shield """
        img = img or self.img
        lcd.display(img)

    def find_lines(self, img=None):
        """ Finds lines in images """
        img = img or self.img
        for line in img.find_lines(threshold = self.threshold,
                                   theta_margin = self.theta_margin,
                                   rho_margin = self.rho_margin):
            if (self.min_degree <= line.theta()) and (line.theta() <= self.max_degree):
                img.draw_line(l.line(), color = self.color)
                #print(l)


timer = VeloTimer()
while(True):
    timer.clock.tick() # Update the FPS clock.
    timer.snapshot() # Take a picture store
    #timer.render()  # 25hz
    print(timer.clock.fps(), sensor.__read_reg(0xBB))
