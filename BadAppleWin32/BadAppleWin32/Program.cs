using System.Runtime.CompilerServices;
using GleamTech.Drawing;
using GleamTech.VideoUltimate;

const int WIDTH = 34;
const int HEIGHT = 11;
const byte BYTE_ONE = 255;
const byte BYTE_ZERO = 0;
const byte BYTE_FRAME_PIXEL = 0;
const byte BYTE_FRAME_SCREENSHOT = 1;

var outputPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Desktop), "framedata.bapl");
using var outputStream = File.Open(outputPath, FileMode.Create, FileAccess.ReadWrite);
using var videoReader = new VideoFrameReader(File.OpenRead("downscaled.mp4"));
var count = 100;

var buffer = new Span<bool>(new bool[WIDTH * HEIGHT]);

for (var i = 0; videoReader.Read(); i++)
{
    Console.WriteLine(i);
    using var frame = videoReader.GetFrame();
    using var bitmap = frame.ToSystemDrawingBitmap();
    for (byte x = 0; x < WIDTH; x++)
    {
        for (byte y = 0; y < HEIGHT; y++)
        {
            var bufferValue = WIDTH * y + x;
            var color = bitmap.GetPixel(x, y).R > 128;
            if (buffer[bufferValue] != color)
            {
                outputStream.WriteByte(BYTE_FRAME_PIXEL);
                outputStream.WriteByte(x);
                outputStream.WriteByte(y);
                outputStream.WriteByte(color ? BYTE_ONE : BYTE_ZERO);
                buffer[bufferValue] = color;
            }
        }
    }
    outputStream.WriteByte(BYTE_FRAME_SCREENSHOT);
}