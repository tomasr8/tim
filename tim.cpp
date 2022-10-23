#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <string>
#include <chrono>
#include <thread>


using namespace std;
using namespace cv;

void sleepMs(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void printHelp() {
  cout << "tim [-v] FILE\n";
  cout << "Example:\n";
  cout << "\ttim lena.jpg\n";
  cout << "\ttim -v video.mp4\n";
}

void clearTerminal() {
  // https://stackoverflow.com/a/7660837
  cout << "\e[1;1H\e[2J";
}

tuple<int, int> getTerminalSize() {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return tuple<int, int>(w.ws_row, w.ws_col);
}

tuple<int, int> getMaxImageSize() {
  const auto [rows, cols] = getTerminalSize();
  return tuple<int, int>(cols, 2 * rows);
}

// Resize the image so that it fits in the terminal window while keeping its aspect ratio.
Mat resizeImageWithAspectRatio(Mat image, int maxWidth, int maxHeight) {
  const int w = image.cols;
  const int h = image.rows;
  Mat resized;

  const double heightRatio = (double)maxHeight / h;
  const double widthRatio = (double)maxWidth / w;

  // Every character is composed of a lower and an upper 'pixel'.
  // This tends to squish the image along the x-axis.
  // The corrective factor accounts for this by stretching the
  // image width a bit more when resizing.
  double correctiveFactor = 1.2;
  const int newWidth = round(heightRatio * w * correctiveFactor);
  int newHeight = round(widthRatio * h / correctiveFactor);
  // Keep the height a multiple of two, so we don't have to deal with drawing only the top half of characters.
  if (newHeight % 2 == 1) {
    newHeight -= 1;
  }

  if (newWidth <= maxWidth) {
    resize(image, resized, Size(newWidth, maxHeight), INTER_LINEAR);
  } else {
    resize(image, resized, Size(maxWidth, newHeight), INTER_LINEAR);
  }
  return resized;
}

Mat resizeImage(Mat image, int width, int height) {
  Mat resized;
  resize(image, resized, Size(width, height), INTER_LINEAR);
  return resized;
}

tuple<string, string, string> colorToString(Vec3b color) {
  return tuple<string, string, string>(
      to_string(color[0]), to_string(color[1]), to_string(color[2]));
}

string getBackground(Vec3b color) {
  auto [b, g, r] = colorToString(color);
  return "\033[48;2;" + r + ";" + g + ";" + b + "m";
}

string getForeground(Vec3b color) {
  auto [b, g, r] = colorToString(color);
  const string lowerHalfBlock = "\u2584";
  return "\033[38;2;" + r + ";" + g + ";" + b + "m" + lowerHalfBlock;
}

string reset() {
  return "\033[0m\n";
}

string convertImageToUnicode(Mat image, int maxWidth, int maxHeight) {
  Mat resized = resizeImageWithAspectRatio(image, maxWidth, maxHeight);

  const string lowerHalfBlock = "\u2584";
  string out = "";

  for (int y = 0; y < resized.rows; y += 2) {
    for (int x = 0; x < resized.cols; x++) {
      Vec3b bgColor = resized.at<Vec3b>(y, x);
      Vec3b fgColor = resized.at<Vec3b>(y + 1, x);

      out += getBackground(bgColor);
      out += getForeground(fgColor);
    }
    out += reset();
  }
  return out;
}

void drawImage(Mat image) {
  const auto [maxWidth, maxHeight] = getMaxImageSize();
  Mat resized = resizeImageWithAspectRatio(image, maxWidth, maxHeight);

  const string lowerHalfBlock = "\u2584";
  string out = "";

  for (int y = 0; y < resized.rows; y += 2) {
    for (int x = 0; x < resized.cols; x++) {
      Vec3b bgColor = resized.at<Vec3b>(y, x);
      Vec3b fgColor = resized.at<Vec3b>(y + 1, x);

      out += getBackground(bgColor);
      out += getForeground(fgColor);
    }
    out += reset();
  }

  cout << out;
}

void drawVideo(VideoCapture cap) {
  const int fps = cap.get(CAP_PROP_FPS);
  while (true) {
    chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    const auto [maxWidth, maxHeight] = getMaxImageSize();
    Mat frame;
    cap >> frame;

    if (frame.empty())
      break;

    string image = convertImageToUnicode(frame, maxWidth, maxHeight);
    clearTerminal();
    cout << image;
    chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    int durationMs = chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    sleepMs(round(1e3/fps) - durationMs);
  }

  cap.release();
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cout << "Incorrect parameters\n\n";
    printHelp();
    return -1;
  }

  if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
    printHelp();
    return 0;
  } else if (strcmp(argv[1], "-v") == 0) {
    if (argc < 3) {
      cout << "Incorrect parameters\n\n";
      printHelp();
      return -1;
    }
    VideoCapture cap(argv[2]);
    if (!cap.isOpened()) {
      cout << "Error opening video stream or file" << endl;
      return -1;
    }
    drawVideo(cap);
  } else {
    Mat image = imread(argv[1]);
    if (image.empty()) {
      cout << "Failed to load the image!\n";
      return -1;
    }
    drawImage(image);
  }

  return 0;
}