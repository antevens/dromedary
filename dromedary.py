# Velodrome marking/line sensor
import sensor, image, time, lcd, pyb, micropython, ulab, utime, math
class VeloTimer():
    """
    Primary class to find lines and send signals
    See documentation for the imaging sensor:
    https://github.com/antevens/dromedary/blob/master/documents/MT9V034.pdf
    """
    def __init__(self, draw_stats=False,
                       draw_lines=False,
                       draw_lap_times=False,
                       draw_timer=False,
                       draw_line_stats=False,
                       save_first_frame=False,
                       flip_text=False,
                       mirror_text=False,
                       rotate_text=0,
                       flip_travel_direction=False):
        # Setup hardware
        self.red_led = pyb.LED(1)
        self.green_led = pyb.LED(2)
        self.blue_led = pyb.LED(3)
        self.infra_led = pyb.LED(4)
        self.usb_serial = pyb.USB_VCP() # Serial Port

        # Auto gain and white balance settings
        #sensor.set_auto_gain(False) # must be turned off for color tracking
        #sensor.set_auto_whitebal(False) # must be turned off for color tracking

        # Set line finding parameters
        self.min_degree = 45
        self.max_degree = 135
        self.threshold = 1000
        self.theta_margin = 25 # Max angle of lines to be merged and considered one
        self.rho_margin = 25 # Max spacing between lines along the rho axis
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

        # Default travel direction is from top to bottom
        self.flip_travel_direction=flip_travel_direction

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
        self.draw_lap_times = draw_lap_times
        self.draw_timer = draw_timer
        self.draw_line_stats = draw_line_stats
        self.line_draw_color = (255, 0, 0)
        self._fps = None
        self._known_lines = []
        self._lap_timestamp = None
        self.lap_timestamps = []
        self._lap_notification_timestamp = None
        self.lap_notification_timeout = 5000
        self.save_first_frame = save_first_frame
        self.flip_text=flip_text
        self.mirror_text=mirror_text
        self.rotate_text=rotate_text

    def __del__(self):
        """ Clean up timers """
        self._timer.deinit()

    def draw_line_stat(self, img=None):
        img.draw_string(10,20, 'Laps: {:d}'.format(len(self.lap_timestamps)), string_vflip=self.flip_text, string_hmirror=self.mirror_text, string_rotation=self.rotate_text)

    def draw_time(self, img=None):
        img = img or self.img
        if self.lap_timestamps:
            img.draw_string(10, int(sensor.height() / 2 + 10), '{:d}ms'.format(utime.ticks_diff(utime.ticks_ms(),self.lap_timestamps[-1])), string_vflip=self.flip_text, string_hmirror=self.mirror_text,string_rotation=self.rotate_text)

    def draw_laps(self, img=None):
        img = img or self.img
        if len(self.lap_timestamps) > 1 and utime.ticks_diff(utime.ticks_add(self.lap_timestamps[-1], self.lap_notification_timeout), utime.ticks_ms()) >= 0:
            lap_time = utime.ticks_diff(self.lap_timestamps[-1], self.lap_timestamps[-2])
            hours, remainder = divmod(lap_time, 3600000)
            minutes, remainder = divmod(remainder, 60000)
            seconds, milliseconds = divmod(remainder, 1000)
            lap_delta='{:03}'.format(milliseconds)

            if seconds:
               lap_delta = '{:02}.'.format(int(seconds)) + lap_delta
               if minutes:
                  lap_delta = '{:02}:'.format(int(minutes)) + lap_delta
                  if hours:
                     lap_delta = '{:02}:'.format(int(hours)) + lap_delta
               else:
                  lap_delta = lap_delta + 's'
            else:
               lap_delta = lap_delta + 'ms'

            lap_delta = 'Lap: ' + lap_delta

            img.draw_string(10, int(sensor.height() / 2), lap_delta, string_vflip=self.flip_text, string_hmirror=self.mirror_text, string_rotation=self.rotate_text)
            return lap_time

    def draw_fps(self, img=None):
        """ Draws the camera FPS on an image """
        img = img or self.img
        if self._fps:
            img.draw_string(0,0, "{:03.0f}fps".format(self._fps), string_vflip=self.flip_text, string_hmirror=self.mirror_text, string_rotation=self.rotate_text)
            return self._fps

    def draw_exposure(self, img=None, scale=None):
        """ Draws camera exposure lines on an image """
        img = img or self.img
        scale = scale or self.scale
        img.draw_string(0, int(sensor.height() * scale - 10), "{:03d}el".format(self.get_exposure_lines()), string_vflip=self.flip_text, string_hmirror=self.mirror_text, string_rotation=self.rotate_text)
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
        if self.draw_line_stats:
            self.draw_line_stat(img)
        if self.draw_lap_times:
            self.draw_laps(img)
        if self.draw_timer:
            self.draw_time(img)
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
        Finds lines in images using a Hough transform

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

            # Compare to existing known lines
            for iteration, existing_line in self._known_lines:
                delta_array = ulab.array(line.line()) - ulab.array(existing_line.line())
                # Compare each coordinate, x1,x2,y1,y2
                for delta in delta_array:
                    if abs(delta) > self.line_id_max_delta:
                        # Coordinate delta too high, not a match
                        break
                else:
                    # We found a match, no need to process more lines, update location
                    self._known_lines.remove([iteration, existing_line])
                    self._known_lines.append([0,line])

                    # Assumes travel direction is up for the sensor
                    start_x = min(existing_line.x1(), existing_line.x2(), line.x1(), line.x2())
                    start_y = min(existing_line.y1(), line.y2())
                    width = max(existing_line.x1(), existing_line.x2(), line.x1(), line.x2())
                    height = max(line.y2(), line.y1())
                    if width and height:
                        if not self.flip_travel_direction:
                            bounding_box = (start_x, start_y-height, width, height)
                        else:
                            bounding_box = (start_x, start_y, width, height)
                        img.draw_rectangle(bounding_box)
                        if all(bounding_box):
                            stats = img.copy(bounding_box).get_statistics()
                        #print("mean", stats.mean())

                        #print("median", stats.median())
                        #print("mode", stats.mode())
                        #print("stdev", stats.stdev())
                            print("lq", stats.lq())
                        #print("uq", stats.uq())

                      # add smart code to detect rising/falling
                      #CREATE non square bounding box and get average colour, see about changing sensor mode
                    break
            else:
                # If no matching line was found it must be new
                self.green_led.on()
                self.lap_pin.value(pyb.Pin.PULL_DOWN)
                self.lap_timestamps.append(utime.ticks_ms())
                print("New line found")
                self._known_lines.append([0, line])
                if self.save_first_frame:
                    img.save(str(utime.ticks_ms()) + ".jpg")

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

timer = VeloTimer(draw_stats=True,
                  draw_lines=True,
                  draw_lap_times=True,
                  draw_timer=True,
                  draw_line_stats=True,
                  save_first_frame=True)
while(True):
    timer.snapshot() # Take a picture
    timer.find_lines() # Process the picture
