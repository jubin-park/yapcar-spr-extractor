require "gosu"

WIDTH, HEIGHT = 800, 600

class Welcome < (Example rescue Gosu::Window)
  # https://stackoverflow.com/questions/73092064/is-there-a-way-to-convert-16-bit-color-to-24-bit-color-efficiently-while-avoidin

  def initialize
    super WIDTH, HEIGHT
    button_file = File.new('../samples/login_panel_notice.SPR_00', 'rb')
    button_size = button_file.read(4).bytes
    @button_width = button_size[0] + (button_size[1] << 8)
    @button_height = button_size[2] + (button_size[3] << 8)
    @button_data = button_file.read.bytes
  end

  def draw
    for i in 0...@button_data.length / 2
      x = i % @button_width
      y = i / @button_width
      color565 = @button_data[i * 2] | (@button_data[i * 2 + 1] << 8)
      r8 = ((((color565 >> 11) & 0x1F) * 527) + 23) >> 6
      g8 = ((((color565 >> 5) & 0x3F) * 259) + 33) >> 6
      b8 = (((color565 & 0x1F) * 527) + 23) >> 6
      color = Gosu::Color.argb(0xff, r8, g8, b8)
      Gosu.draw_rect(x, y, 1, 1, color)
    end
  end
end

Welcome.new.show if __FILE__ == $0
