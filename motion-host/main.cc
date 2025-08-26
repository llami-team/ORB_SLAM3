#include <csignal>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include "base64.h"
#include "socket.h"

#include <opencv2/core/core.hpp>

#include <System.h>

ORB_SLAM3::System *slam = nullptr;
cv::VideoCapture *cap = nullptr;

std::vector<ORB_SLAM3::IMU::Point> imuMeasurements;

// Forward declarations for cleanup functions
void cleanup_resources();

// Global variable to be set by signal handler
volatile sig_atomic_t g_should_stop = 0;

extern "C" void sigterm_handler(int signum) {
  // Use sig_atomic_t which is safe for signal handlers
  g_should_stop = 1;

  // Don't call socket_close from signal handler as it might not be async-safe
  // We'll handle cleanup in the main thread
}

// This function will be called from the main thread to safely clean up
// resources
void cleanup_resources() {
  if (slam) {
    delete slam;
    slam = nullptr;
  }

  if (cap) {
    delete cap;
    cap = nullptr;
  }
}

// Socket message handler callback
void handle_socket_message(char *buffer, int length) {
  // Null-terminate the buffer to ensure it's a valid string
  buffer[length] = '\0';

  // Process socket messages similar to stdin messages
  std::string message(buffer);

  if (message == "exit") {
    g_should_stop = 1;
    return;
  }

  // imu message format: imu <base64>
  // (acc_x, acc_y, acc_z, ang_vel_x, ang_vel_y, ang_vel_z, timestamp) in fp32
  if (message.substr(0, 3) == "imu") {
    std::vector<uint8_t> imuData = base64_decode(message.substr(4));

    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int64_t timestamp_ll = ts.tv_sec * 1000000000LL + ts.tv_nsec;
    double timestamp = (double)timestamp_ll * 1e-9;

    // convert to fp32
    float acc_x = *(float *)(&imuData[0]);
    float acc_y = *(float *)(&imuData[4]);
    float acc_z = *(float *)(&imuData[8]);
    float ang_vel_x = *(float *)(&imuData[12]);
    float ang_vel_y = *(float *)(&imuData[16]);
    float ang_vel_z = *(float *)(&imuData[20]);

    imuMeasurements.emplace_back(acc_x, acc_y, acc_z, ang_vel_x, ang_vel_y,
                                 ang_vel_z, timestamp);
  }
}

int main(int argc, char **argv) {
  if (argc < 5) {
    std::cerr << "Usage: " << argv[0]
              << " path_to_vocabulary path_to_settings camera_rtsp_url port\n";
    return 1;
  }

  // Initialize socket server
  int port = std::stoi(argv[4]);
  int socket_fd = socket_create(port);
  if (socket_fd < 0) {
    std::cerr << "Failed to create socket server on port " << port << "\n";
    return 1;
  }
  std::cout << "Socket server started on port " << port << "\n";

  // Set message listener
  set_message_listener(handle_socket_message);

  // Start accepting connections in a separate thread
  std::thread accept_thread([&]() {
    while (g_should_stop == 0) {
      int client = accept_connection();
      if (client == -2) {
        std::cerr << "Accept error occurred\n";
      }
    }
  });

  ORB_SLAM3::Verbose::SetTh(ORB_SLAM3::Verbose::VERBOSITY_VERY_VERBOSE);
  slam = new ORB_SLAM3::System(argv[1], argv[2],
                               ORB_SLAM3::System::IMU_MONOCULAR, false);
  cap = new cv::VideoCapture(argv[3]);

  signal(SIGTERM, sigterm_handler);
  signal(SIGINT, sigterm_handler);
  // imuMeasurements is now declared globally

  size_t frameId = 0;
  timespec ts;

  std::thread slamThread([&]() {
    while (g_should_stop == 0) {
      if (!cap->isOpened()) {
        std::cout << "Error opening video stream or file" << "\n";
        break;
      }

      cv::Mat image;
      cap->read(image);
      if (image.empty()) {
        break;
      }

      clock_gettime(CLOCK_REALTIME, &ts);
      int64_t timestamp_ll = ts.tv_sec * 1000000000LL + ts.tv_nsec;
      double timestamp = (double)timestamp_ll * 1e-9;

      if (frameId++ % 3 != 0) { // restrict frame to 10fps
        continue;
      }

      Sophus::SE3f pose =
          slam->TrackMonocular(image, timestamp, imuMeasurements);
      imuMeasurements.clear();

      std::vector<uint8_t> poseData(sizeof(float) * 16);

      memcpy(poseData.data(), pose.matrix().data(), sizeof(float) * 16);
      std::string poseMessage = "pose " + base64_encode(poseData);

      // Send pose data to connected clients
      send_message(poseMessage.c_str(), static_cast<int>(poseMessage.length()));
    }
  });

  // Wait for signal to stop
  std::cout << "System running. Enter 'exit' in terminal or send 'exit' via "
               "socket to stop.\n";

  // Simple loop to check for exit command from stdin (keeping this for
  // convenience)
  std::string line;
  while (g_should_stop == 0) {
    // Non-blocking check for stdin input
    if (std::cin.rdbuf()->in_avail()) { // Check if there's input available
      std::getline(std::cin, line);
      if (line == "exit") {
        g_should_stop = 1;
        break;
      }
    }

    // Sleep to avoid busy waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  g_should_stop = 1;
  slamThread.join();

  socket_close();
  accept_thread.join();

  slam->Shutdown();
  cleanup_resources();

  return 0;
}
