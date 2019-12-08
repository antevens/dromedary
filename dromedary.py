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

        # Set line finding parameters
        self.min_degree = 45
        self.max_degree = 135
        self.threshold = 1300
        self.theta_margin = 45
        self.rho_margin = 45
        self.x_stride = 2
        self.y_stride = 8

        # Configure IO pins for signaling
        self.action_pin = pyb.Pin('P7', pyb.Pin.OUT_OD, pyb.Pin.PULL_NONE)
        self.page_pin = pyb.Pin('P8', pyb.Pin.OUT_OD, pyb.Pin.PULL_NONE)
        self.lap_pin = pyb.Pin('P9', pyb.Pin.OUT_OD, pyb.Pin.PULL_NONE)

        # Configure the imaging sensor
        sensor.reset() # Initialize the sensor
        sensor.set_pixformat(sensor.GRAYSCALE) # Set pixel format
        sensor.set_framesize(sensor.QQQVGA) # Set frame size
        sensor.set_auto_exposure(True, exposure_us=5000) # Smaller means faster
        sensor.skip_frames(time = 2000) # Wait for settings take effect

        # Delta value in pixels for x/y for tracking new lines
        self.line_id_max_delta = 40

        # Max frames without a line before line history is cleared
        self.frames_before_line_purge = 200 # should be inverse -> sensor.get_exposure_us / 25

        # Configure clock for tracking FPS
        self.clock = time.clock()
        # Configure the lcd screen.
        lcd.init()
        # Initialize image buffer
        self.img = sensor.snapshot()

        #Allocate memory for exceptions in async/timer driven code
        micropython.alloc_emergency_exception_buf(100) # Debugging only

        # Allocation for interrupt callbacks
        self._timer = pyb.Timer(13)
        self._timer.init(freq=10)
        self._timer.callback(self._cb)
        self._render_ref = self.render
        self._pin_reset_ref = self.pin_reset

        # Scale, sensor to screen
        self.scale = 1.5

        # Show performance/debug statistics/info
        self.draw_stats = draw_stats
        self.draw_lines = draw_lines
        self.line_draw_color = (255, 0, 0)
        self._fps = None
        self._known_lines = []

    def __del__(self):
        """ Clean up timers """
        self._timer.deinit()

    def draw_fps(self, img=None):
        """ Draws the camera FPS on an image """
        img = img or self.img
        if self._fps:
            img.draw_string(0,0, "{:03.0f}fps".format(self._fps))
        return img

    def draw_exposure(self, img=None, scale=None):
        """ Draws camera exposure lines on an image """
        img = img or self.img
        scale = scale or self.scale
        img.draw_string(0, int(sensor.height() * scale - 10), "{:03d}el".format(self.get_exposure_lines()))
        return img

    def _cb(self, timer):
        """ Callback event handler for rendering to the LCD async """
        micropython.schedule(self._render_ref, timer)
        micropython.schedule(self._pin_reset_ref, timer)

    def snapshot(self):
        """ Takes a snapshot/picture from the image sensor """
        self.clock.tick() # Update the FPS clock.
        self.img = sensor.snapshot()
        self._fps = self.clock.fps()
        return self.img

    def pin_reset(self, timer):
        """ Resets IO pins to None/Default state """
        self.lap_pin.value(pyb.Pin.PULL_NONE)

    def render(self, timer, img=None, scale=None):
        """ Renders an image to the LCD shield """
        img = img or self.img
        scale = scale or self.scale
        if self.draw_stats:
            img = img.copy((0,0,sensor.width(),sensor.height()),scale,scale).to_rgb565(copy=True)
            self.draw_fps(img)
            self.draw_exposure(img)
        if self.draw_lines:
            for iterations, line in self._known_lines:
                scaled_line = ulab.array(line.line()) * scale
                img.draw_line(int(scaled_line[0]),
                              int(scaled_line[1]),
                              int(scaled_line[2]),
                              int(scaled_line[3]),
                              color = self.line_draw_color)
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
        lines_present = []
        for line in img.find_lines(threshold=self.threshold,
                                   x_stride=self.x_stride,
                                   y_stride=self.y_stride,
                                   theta_margin=self.theta_margin,
                                   rho_margin=self.rho_margin):
            # Skip to next line if line is below/above limits
            if self.min_degree and line.theta() < self.min_degree:
                continue
            if self.max_degree and line.theta() > self.max_degree:
                continue

            # Filtered list of lines within theta limits
            lines_present.append(line)

            # Skip to next line if line is below/above limits
            for iteration, existing_line in self._known_lines:
               delta_array = ulab.array(line.line()) - ulab.array(existing_line.line())
               # Compare each coordinate, x1,x2,y1,y2
               for delta in delta_array:
                  if abs(delta) > self.line_id_max_delta:
                      # Coordinate delta too high, not a match
                      break
               else:
                   # Compare each coordinate, x1,x2,y1,y2
                   self._known_lines.remove([iteration, existing_line])
                   self._known_lines.append([0,line])
                   # No need to process more lines
                   break
            else:
                # If no matching line was found it must be new
                self.green_led.on()
                self.lap_pin.value(pyb.Pin.PULL_DOWN)
                #self.lap_pin.value(pyb.Pin.PULL_NONE)
                print("New line found")
                self._known_lines.append([0, line])
        for place, iteration_line in enumerate(self._known_lines):
            if iteration_line[1] not in lines_present:
                if iteration_line[0] >= self.frames_before_line_purge:
                     self._known_lines.remove(iteration_line)
                     self.lap_pin.value(pyb.Pin.PULL_NONE)
                     print("Purged line")
                else:
                    iteration_line[0] += 1
        if not self._known_lines:
            self.green_led.off()
        return lines_present

timer = VeloTimer(draw_stats=True, draw_lines=True)
while(True):
    timer.snapshot() # Take a picture
    timer.find_lines() # Process the picture
