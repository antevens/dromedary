# Velodrome marking/line sensor
import sensor, image, time, lcd, pyb, micropython




class VeloTimer():
    def __init__(self, draw_stats=False):
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
        self.max_degree = 135
        self.threshold = 2000
        self.theta_margin = 25
        self.rho_margin = 25
        self.color = (255, 0, 0)

        # Schedule async LCD shield updates
        micropython.alloc_emergency_exception_buf(100) # Debugging only
        self.render_ref = self.render # Allocation occurs here
        self.tim = pyb.Timer(4) # Assign timer 5
        self.tim.init(freq=10) # Set LCD shield update freq. to 20hz
        self.tim.callback(self.cb)

        # Show performance statistics
        self.draw_stats=draw_stats
        self._fps = None

    def __del__(self):
        """ Clean up timers """
        self.tim.deinit()

    def draw_fps(self, img=None):
        img = img or self.img
        if self._fps:
            img.draw_string(0,0, "{:03.0f}fps".format(self._fps))

    def draw_exposure(self, img=None):
         img = img or self.img
         img.draw_string(0,50, "{:03d}ms".format(timer.get_exposure()))

    def cb(self, timer):
        """ Callback event handler for rendering to the LCD async """
        micropython.schedule(self.render_ref, timer)

    def snapshot(self):
        """ Takes a snapshot from the image sensor """
        self.clock.tick() # Update the FPS clock.
        self.img = sensor.snapshot()
        self._fps = self.clock.fps()

    def render(self, timer, img=None):
        """ Renders an image to the LCD shield """
        img = img or self.img
        if self.draw_stats:
            img = img.copy()
            self.draw_fps(img)
            self.draw_exposure(img)
        lcd.display(img)

    def get_exposure(self):
        return sensor.__read_reg(0xBB)


    def find_lines(self, img=None):
        """ Finds lines in images """
        img = img or self.img
        for line in img.find_lines(threshold=self.threshold,
                                   x_stride=10,
                                   y_stride=4,
                                   theta_margin=self.theta_margin,
                                   rho_margin=self.rho_margin):
            if (self.min_degree <= line.theta()) and (line.theta() <= self.max_degree):
                img.draw_line(line.line(), color = self.color)
                print(line)


timer = VeloTimer(draw_stats=True)
while(True):
    timer.snapshot() # Take a picture store
    timer.find_lines()
