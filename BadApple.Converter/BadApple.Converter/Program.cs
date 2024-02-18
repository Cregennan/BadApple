using System.Runtime.CompilerServices;
using GleamTech.Drawing;
using GleamTech.VideoUltimate;

// Ширина входного видео
const int WIDTH = 34;
// Высота входного видео (с учетом игнорируемой последней строки)
const int HEIGHT = 11;

// Значение байта для БЕЛОГО пикселя
const byte BYTE_ONE = 255;
// Значение байта для ЧЕРНОГО пикселя
const byte BYTE_ZERO = 0;
// Значение байта для команды "ПОКРАСИТЬ ПИКСЕЛЬ"
const byte BYTE_FRAME_PIXEL = 0;
// Значение байта для команды "Сделать скриншот"
const byte BYTE_FRAME_SCREENSHOT = 1;

// Ссылка на выходной файл
var outputPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Desktop), "framedata.bapl");
// Поток выходного файла
using var outputStream = File.Open(outputPath, FileMode.Create, FileAccess.ReadWrite);
// Входной файл
using var videoReader = new VideoFrameReader(File.OpenRead("downscaled-33x12.mp4"));

// Буфер предыдущего кадра
var buffer = new Span<bool>(new bool[WIDTH * HEIGHT]);

//Считываем кадры, пока они доступны
for (var i = 0; videoReader.Read(); i++)
{
    Console.WriteLine(i);
    //Получаем кадр и преобразуем его к Bitmap из .NET
    using var frame = videoReader.GetFrame();
    using var bitmap = frame.ToSystemDrawingBitmap();
    for (byte x = 0; x < WIDTH; x++)
    {
        for (byte y = 0; y < HEIGHT; y++)
        {
            //Индекс пикселя в буфере
            var bufferValue = WIDTH * y + x;
            //Получим значение пикселя (канал значения не имеет, видео черно-белое)
            var color = bitmap.GetPixel(x, y).R > 128;
            if (buffer[bufferValue] != color)
            {
                // Записываем байт команды изменения пикселя
                outputStream.WriteByte(BYTE_FRAME_PIXEL);
                // Записываем данные измененного пикселя
                outputStream.WriteByte(x);
                outputStream.WriteByte(y);
                outputStream.WriteByte(color ? BYTE_ONE : BYTE_ZERO);
                buffer[bufferValue] = color;
            }
        }
    }
    //Записываем байт команды скриншота
    outputStream.WriteByte(BYTE_FRAME_SCREENSHOT);
}