# Velodrome marking/line sensor
import sensor, image, time, lcd, pyb, micropython, ulab

class VeloTimer():
    """
    Primary class to find lines and send signals

    See documentation for the imaging sensor:
    https://github.com/antevens/dromedary/blob/master/documents/MT9V034.pdf
    """
    def __init__(self, draw_stats=False, draw_lines=False):

        # Setup hardware
        self.red_led = pyb.LED(1)
        self.green_led = pyb.LED(2)
        self.blue_led = pyb.LED(3)
        self.infra_led = pyb.LED(4)
        self.usb_serial = pyb.USB_VCP() # Serial Port

        # Delta value in pixels for x/y for tracking new lines
        self.line_id_max_delta = 10

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
        self.theta_margin = 100 #25
        self.rho_margin = 100 #25


        # Schedule async LCD shield updates
        micropython.alloc_emergency_exception_buf(100) # Debugging only
        self.render_ref = self.render # Allocation occurs here
        self.tim = pyb.Timer(4) # Assign timer 5
        self.tim.init(freq=10) # Set LCD shield update freq. to 20hz
        self.tim.callback(self.cb)

        # Show performance/debug statistics/info
        self.draw_stats = draw_stats
        self.draw_lines = draw_lines
        self.line_draw_color = (255, 0, 0)
        self._fps = None

        # Store last known lines
        self._lines = self.find_lines()


    def __del__(self):
        """ Clean up timers """
        self.tim.deinit()

    def draw_fps(self, img=None):
        img = img or self.img
        if self._fps:
            img.draw_string(0,0, "{:03.0f}fps".format(self._fps))
        return img

    def draw_exposure(self, img=None):
         img = img or self.img
         img.draw_string(0,50, "{:03d}el".format(self.get_exposure_lines()))
         return img

    def cb(self, timer):
        """ Callback event handler for rendering to the LCD async """
        micropython.schedule(self.render_ref, timer)

    def snapshot(self):
        """ Takes a snapshot from the image sensor """
        self.clock.tick() # Update the FPS clock.
        self.img = sensor.snapshot()
        self._fps = self.clock.fps()
        return self.img

    def render(self, timer, img=None):
        """ Renders an image to the LCD shield """
        img = img or self.img
        if self.draw_stats:
            img = img.to_rgb565(copy=True)
            self.draw_fps(img)
            self.draw_exposure(img)
            #print(self._fps, self.get_exposure_lines())
        if self.draw_lines:
            for line in self._lines:
                img.draw_line(line.line(), color = self.line_draw_color)
        lcd.display(img)

    def get_exposure_lines(self):
        """
        The exposure measured in row-time from 1 to 2047.
        """
        return sensor.__read_reg(0xBB)


    def get_gain(self):
        """
        The gain measured in gain-units. The gain range is 16 to 63
        (unity gain = 16 gain-units; multiply by 1/16 to get the true
        gain).
        """
        return sensor.__read_reg(0xBA)

    def find_lines(self, img=None, min_degree=None, max_degree=None):
        """
        Finds lines in images

        `threshold` controls how many lines in the image are found. Only lines with
        edge difference magnitude sums greater than `threshold` are detected...

        More about `threshold` - each pixel in the image contributes a magnitude value
        to a line. The sum of all contributions is the magintude for that line. Then
        when lines are merged their magnitudes are added togheter. Note that `threshold`
        filters out lines with low magnitudes before merging. To see the magnitude of
        un-merged lines set `theta_margin` and `rho_margin` to 0...

        `theta_margin` and `rho_margin` control merging similar lines. If two lines
        theta and rho value differences are less than the margins then they are merged.


        All line objects have a `theta()` method to get their rotation angle in degrees.
        You can filter lines based on their rotation angle.

        """

        img = img or self.img
        all_lines = img.find_lines(threshold=self.threshold,
                               x_stride=8,
                               y_stride=4,
                               theta_margin=self.theta_margin,
                               rho_margin=self.rho_margin)

        filtered_lines = []
        for line in all_lines:
            if self.min_degree and line.theta() < self.min_degree:
                continue
            if self.max_degree and line.theta() > self.max_degree:
                continue
            filtered_lines.append(line)
            self.red_led.toggle()

            for existing_line in self._lines:
               delta_array = ulab.array(line.line()) - ulab.array(existing_line.line())
               for delta in delta_array:
                  if abs(delta) > self.line_id_max_delta:
                      print("New line")
                      break
               else:
                   print("Same line")
                   break
            else:
                print("Newer Line")


        self._lines = filtered_lines
        return filtered_lines

timer = VeloTimer(draw_stats=True, draw_lines=True)
while(True):
    timer.snapshot() # Take a picture store
    timer.find_lines()
