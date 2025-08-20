#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>

#include <opencv2/core/core.hpp>

#include <System.h>

int main(int argc, char **argv) {
  ORB_SLAM3::System SLAM(argv[1], argv[2], ORB_SLAM3::System::MONOCULAR, false);

  cv::VideoCapture cap(argv[3]);

  size_t frame_id = 0;

  while (true) {
    while (!cap.isOpened()) {
      std::cout << "Error opening video stream or file" << std::endl;
      return -1;
    }

    cv::Mat im;
    cap >> im;
    if (im.empty()) {
      break;
    }

    if (frame_id++ % 3 != 0) { // restrict frame to 10fps
      continue;
    }

    Sophus::SE3f pose = SLAM.TrackMonocular(im, 0);
    std::cout << pose.matrix() << std::endl;
  }

  return 0;
}
