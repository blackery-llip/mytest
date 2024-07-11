根据 gnss_grpc_proxy.cpp 的内容，特别是 ReadNmeaFromLocalFile() 和 ReadGnssRawMeasurement() 函数的实现，可以推导出 FLAGS_gnss_file_path 文件的内容可能是以下格式的NMEA数据和GNSS原始测量数据交替存储：

假设的 FLAGS_gnss_file_path 文件内容推导：
假设该文件中存储了交替的 NMEA 数据和 GNSS 原始测量数据，如下所示：

mathematica
复制代码
$GPGGA,084502.00,3723.2475,N,12158.3416,W,1,04,2.0,100.0,M,-34.0,M,,*47
$GPRMC,084502.00,A,3723.2475,N,12158.3416,W,0.05,162.3,270598,12.2,E,A*1C
# Raw,1568547020000000000,18,20000,300000000,10000000,1000000,0.01,0.001,0,1,0.01,A,100000000,1000,40,1.0,0.5,0,10,0.5,0,1575.42e6,1000,0.1,2,1.0,1,40,1,10,1.0,10,0,2,40,1.0
$GPGGA,084503.00,3723.2475,N,12158.3416,W,1,04,2.0,100.0,M,-34.0,M,,*46
$GPRMC,084503.00,A,3723.2475,N,12158.3416,W,0.05,162.3,270598,12.2,E,A*1D
# Raw,1568547030000000000,18,20000,300000000,10000000,1000000,0.01,0.001,0,1,0.01,A,100000000,1000,40,1.0,0.5,0,10,0.5,0,1575.42e6,1000,0.1,2,1.0,1,40,1,10,1.0,10,0,2,40,1.0
$GPGGA,084504.00,3723.2475,N,12158.3416,W,1,04,2.0,100.0,M,-34.0,M,,*45
$GPRMC,084504.00,A,3723.2475,N,12158.3416,W,0.05,162.3,270598,12.2,E,A*1E
# Raw,1568547040000000000,18,20000,300000000,10000000,1000000,0.01,0.001,0,1,0.01,A,100000000,1000,40,1.0,0.5,0,10,0.5,0,1575.42e6,1000,0.1,2,1.0,1,40,1,10,1.0,10,0,2,40,1.0
解释：
NMEA 数据：

每条 NMEA 数据行以 $GPGGA 或者 $GPRMC 开头，包含 GPS 定位信息，如时间、经纬度、海拔等。
GNSS 原始测量数据：

每条原始测量数据以 # Raw 开头，后跟一系列测量参数，例如 TimeNanos、State、SnrInDb 等。
交替存储：

文件中交替存储了 NMEA 数据和 GNSS 原始测量数据，每个数据块都通过空行或特定的分隔符分隔开来。
文件结构：

每组数据可能是按照时间顺序存储的，以便模拟实时的 GNSS 数据输入。
ReadNmeaFromLocalFile() 和 ReadGnssRawMeasurement() 函数会按照一定的逻辑读取这些数据，并将其缓存在 cached_nmea 和 cached_gnss_raw 变量中供后续处理使用。
这样的文件内容结构符合了在 gnss_grpc_proxy.cpp 中实现的读取逻辑和数据处理需求。

请你再次确认
ChatGPT
根据 gnss_grpc_proxy.cpp 的代码逻辑，重新推导 FLAGS_gnss_file_path 文件的可能内容如下：

可能的 FLAGS_gnss_file_path 文件内容推导：
假设文件中存储了交替的 NMEA 数据和 GNSS 原始测量数据，格式如下：

mathematica
复制代码
$GPGGA,084502.00,3723.2475,N,12158.3416,W,1,04,2.0,100.0,M,-34.0,M,,*47
$GPRMC,084502.00,A,3723.2475,N,12158.3416,W,0.05,162.3,270598,12.2,E,A*1C
# Raw,1568547020000000000,18,20000,300000000,10000000,1000000,0.01,0.001,0,1,0.01,A,100000000,1000,40,1.0,0.5,0,10,0.5,0,1575.42e6,1000,0.1,2,1.0,1,40,1,10,1.0,10,0,2,40,1.0
$GPGGA,084503.00,3723.2475,N,12158.3416,W,1,04,2.0,100.0,M,-34.0,M,,*46
$GPRMC,084503.00,A,3723.2475,N,12158.3416,W,0.05,162.3,270598,12.2,E,A*1D
# Raw,1568547030000000000,18,20000,300000000,10000000,1000000,0.01,0.001,0,1,0.01,A,100000000,1000,40,1.0,0.5,0,10,0.5,0,1575.42e6,1000,0.1,2,1.0,1,40,1,10,1.0,10,0,2,40,1.0
$GPGGA,084504.00,3723.2475,N,12158.3416,W,1,04,2.0,100.0,M,-34.0,M,,*45
$GPRMC,084504.00,A,3723.2475,N,12158.3416,W,0.05,162.3,270598,12.2,E,A*1E
# Raw,1568547040000000000,18,20000,300000000,10000000,1000000,0.01,0.001,0,1,0.01,A,100000000,1000,40,1.0,0.5,0,10,0.5,0,1575.42e6,1000,0.1,2,1.0,1,40,1,10,1.0,10,0,2,40,1.0
解释：
NMEA 数据：

每条 NMEA 数据行以 $GPGGA 或者 $GPRMC 开头，包含 GPS 定位信息，如时间、经纬度、海拔等。
GNSS 原始测量数据：

每条原始测量数据以 # Raw 开头，后跟一系列测量参数，例如 TimeNanos、State、SnrInDb 等。
交替存储：

文件中交替存储了 NMEA 数据和 GNSS 原始测量数据，每个数据块都通过空行或特定的分隔符分隔开来。
文件结构：

每组数据可能是按照时间顺序存储的，以便模拟实时的 GNSS 数据输入。
ReadNmeaFromLocalFile() 和 ReadGnssRawMeasurement() 函数会按照一定的逻辑读取这些数据，并将其缓存在 cached_nmea 和 cached_gnss_raw 变量中供后续处理使用。
这种结构和格式符合了 gnss_grpc_proxy.cpp 中的数据处理逻辑和读取要求。



GPGGA和GPRMC是什么
ChatGPT
GPGGA 和 GPRMC 是两种常见的 NMEA 0183 格式的 GPS 数据消息类型，用于在GPS设备和接收器之间交换位置、时间和速度等信息。这些消息通常用于导航和定位系统中，以下是它们的解释：

GPGGA（Global Positioning System Fix Data）：

GPGGA 消息提供了定位解的基本信息，包括位置、时间、海拔高度和卫星信息。
示例：
bash
复制代码
$GPGGA,084502.00,3723.2475,N,12158.3416,W,1,04,2.0,100.0,M,-34.0,M,,*47
084502.00：时间（时分秒）
3723.2475,N：纬度（37度23.2475分北纬）
12158.3416,W：经度（121度58.3416分西经）
1：GPS定位状态（1=实时定位，2=差分GPS，6=估算）
04：使用卫星数量
2.0：HDOP（水平精度因子）
100.0,M：海拔高度（米）
-34.0,M：大地水准面的高度（海平面以下为负值）
*47：校验和
GPRMC（Recommended Minimum Specific GNSS Data）：

GPRMC 消息包含了定位、速度、日期和时间信息，通常用于导航和时间同步。
示例：
mathematica
复制代码
$GPRMC,084502.00,A,3723.2475,N,12158.3416,W,0.05,162.3,270598,12.2,E,A*1C
084502.00：时间（时分秒）
A：定位状态（A=有效，V=无效）
3723.2475,N：纬度（37度23.2475分北纬）
12158.3416,W：经度（121度58.3416分西经）
0.05：速度（节）
162.3：航向角
270598：日期（DDMMYY）
12.2,E：磁偏角
A*1C：校验和
这些消息通过NMEA 0183协议标准化，是GPS设备和应用程序之间常用的通信方式，以实现位置和时间数据的传输和解析。

RAW数据又是什么意思
ChatGPT
在 GNSS (Global Navigation Satellite System) 领域中，"RAW" 数据通常指原始测量数据，它包含了来自卫星的信号接收和处理过程中的详细信息。这些原始数据可以包括接收到的卫星信号强度、到达时间、卫星位置等信息，具体含义取决于具体的 GNSS 系统（如 GPS、GLONASS、Galileo 等）以及接收设备的能力和设置。

原始测量数据（RAW data）对于高精度定位和导航应用非常重要，因为它们允许接收设备进行更精确的位置解算和时间同步。通过分析和处理原始测量数据，可以实现更高精度的位置定位、时间同步和其他导航相关的应用。

在你提到的代码和上下文中，ReadGnssRawMeasurement() 函数的作用就是从指定的文件（通过 FLAGS_gnss_file_path 指定）中读取这些原始测量数据，并进一步处理和利用这些数据。

RAW数据有数据格式吗
ChatGPT
"RAW" 数据本身通常是指未经处理或解析的原始数据，因此它可能没有固定的标准格式。具体的 RAW 数据格式会依赖于所使用的 GNSS 系统（比如 GPS、GLONASS、Galileo 等）以及接收设备的具体配置和协议。

在 GNSS 数据处理中，通常会根据不同的应用需求定义数据的格式和结构。一般来说，原始测量数据可能包含以下一些基本信息：

时间戳信息：标识测量数据收集的时间。
卫星相关信息：每个卫星的标识号、接收信号强度等。
接收设备状态：设备的运行状态、收集到的信号质量等。
具体的 RAW 数据格式通常由设备或应用程序的制造商或开发者定义和文档化，以确保数据可以正确地被解析和使用。在实际应用中，解析 RAW 数据可能需要依赖特定的软件库或算法，以从原始的二进制或文本数据中提取出有用的导航或位置信息。


#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/server_posix.h>

#include "gnss_grpc_proxy.grpc.pb.h"

#include <signal.h>

#include <chrono>
#include <deque>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <gflags/gflags.h>

#include <common/libs/fs/shared_buf.h>
#include <common/libs/fs/shared_fd.h>
#include <common/libs/fs/shared_select.h>
#include <host/libs/config/cuttlefish_config.h>
#include <host/libs/config/logging.h>
#include <queue>

using gnss_grpc_proxy::GnssGrpcProxy;
using gnss_grpc_proxy::SendGpsReply;
using gnss_grpc_proxy::SendGpsRequest;
using gnss_grpc_proxy::SendGpsCoordinatesReply;
using gnss_grpc_proxy::SendGpsCoordinatesRequest;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

DEFINE_int32(gnss_in_fd,
             -1,
             "File descriptor for the gnss's input channel");
DEFINE_int32(gnss_out_fd,
             -1,
             "File descriptor for the gnss's output channel");

DEFINE_int32(fixed_location_in_fd, -1,
             "File descriptor for the fixed location input channel");
DEFINE_int32(fixed_location_out_fd, -1,
             "File descriptor for the fixed location output channel");

DEFINE_int32(gnss_grpc_port,
             -1,
             "Service port for gnss grpc");
DEFINE_string(gnss_grpc_socket, "", "Service socket path for gnss grpc");

DEFINE_string(gnss_file_path, "",
              "gnss raw measurement file path for gnss grpc");
DEFINE_string(fixed_location_file_path, "",
              "fixed location file path for gnss grpc");

constexpr char CMD_GET_LOCATION[] = "CMD_GET_LOCATION";
constexpr char CMD_GET_RAWMEASUREMENT[] = "CMD_GET_RAWMEASUREMENT";
constexpr char END_OF_MSG_MARK[] = "\n\n\n\n";

constexpr uint32_t GNSS_SERIAL_BUFFER_SIZE = 4096;

std::string GenerateGpsLine(const std::string& dataPoint) {
  std::string unix_time_millis =
      std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count());
  std::string formatted_location =
      std::string("Fix,GPS,") + dataPoint + "," +
      std::string("0.000000,3.790092,0.000000,") + unix_time_millis + "," +
      std::string("0.086023256,0.0,11529389988248");

  return formatted_location;
}
// Logic and data behind the server's behavior.
class GnssGrpcProxyServiceImpl final : public GnssGrpcProxy::Service {
  public:
   GnssGrpcProxyServiceImpl(cuttlefish::SharedFD gnss_in,
                            cuttlefish::SharedFD gnss_out,
                            cuttlefish::SharedFD fixed_location_in,
                            cuttlefish::SharedFD fixed_location_out)
       : gnss_in_(gnss_in),
         gnss_out_(gnss_out),
         fixed_location_in_(fixed_location_in),
         fixed_location_out_(fixed_location_out) {
          //Set the default GPS delay to 1 second
          fixed_locations_delay_=1000;
         }

    //传入的request是一个string
   Status SendGps(ServerContext* context, const SendGpsRequest* request,
                  SendGpsReply* reply) override {
     reply->set_reply("Received gps record");
     std::lock_guard<std::mutex> lock(cached_fixed_location_mutex);
     cached_fixed_location = request->gps();
     return Status::OK;
   }


  std::string ConvertCoordinate(gnss_grpc_proxy::GpsCoordinates coordinate){
    std::string latitude = std::to_string(coordinate.latitude());
    std::string longitude = std::to_string(coordinate.longitude());
    std::string elevation = std::to_string(coordinate.elevation());
    std::string result = latitude + "," + longitude + "," + elevation;
    return result;
  }

   Status SendGpsVector(ServerContext* context,
                        const SendGpsCoordinatesRequest* request,
                        SendGpsCoordinatesReply* reply) override {
     reply->set_status(SendGpsCoordinatesReply::OK);//update protobuf reply
     {
       std::lock_guard<std::mutex> lock(fixed_locations_queue_mutex_);
       // Reset local buffers
       fixed_locations_queue_ = {};
       // Make a local copy of the input buffers
       for (auto loc : request->coordinates()) {
         fixed_locations_queue_.push(ConvertCoordinate(loc));
       }
       fixed_locations_delay_ = request->delay();
     }

     return Status::OK;
   }

    //将cached_fixed_location固定值发送到fixed_location_in_
    void sendToSerial() {
      std::lock_guard<std::mutex> lock(cached_fixed_location_mutex);
      ssize_t bytes_written = cuttlefish::WriteAll(
          fixed_location_in_, cached_fixed_location + END_OF_MSG_MARK);
      if (bytes_written < 0) {
          LOG(ERROR) << "Error writing to fd: " << gnss_in_->StrError();
      }
    }

    //将cached_gnss_raw发送到gnss_in_
    void sendGnssRawToSerial() {
      std::lock_guard<std::mutex> lock(cached_gnss_raw_mutex);
      if (!isGnssRawMeasurement(cached_gnss_raw)) {
        return;
      }
      if (previous_cached_gnss_raw == cached_gnss_raw) {
        // Skip for same record
        return;
      } else {
        // Update cached data
        LOG(DEBUG) << "Skip same record";
        previous_cached_gnss_raw = cached_gnss_raw;
      }
      ssize_t bytes_written =
          cuttlefish::WriteAll(gnss_in_, cached_gnss_raw + END_OF_MSG_MARK);
      LOG(DEBUG) << "Send Gnss Raw to serial: bytes_written: " << bytes_written;
      if (bytes_written < 0) {
        LOG(ERROR) << "Error writing to fd: " << gnss_in_->StrError();
      }
    }

    void StartServer() {
      // Create a new thread to handle writes to the gnss and to the any client
      // connected to the socket.
      fixed_location_write_thread_ =
          std::thread([this]() { WriteFixedLocationFromQueue(); });
      measurement_read_thread_ =
          std::thread([this]() { ReadMeasurementLoop(); });
      fixed_location_read_thread_ =
          std::thread([this]() { ReadFixedLocLoop(); });
    }

    void StartReadFixedLocationFileThread() {
      // Create a new thread to read fixed_location data.
      fixed_location_file_read_thread_ =
          std::thread([this]() { ReadFixedLocationFromLocalFile(); });
    }

    void StartReadGnssRawMeasurementFileThread() {
      // Create a new thread to read raw measurement data.
      measurement_file_read_thread_ =
          std::thread([this]() { ReadGnssRawMeasurement(); });
    }
    //打开本地文件，朱行解析，每隔一秒将一行的信息赋值给cached_fixed_location
    void ReadFixedLocationFromLocalFile() {
      std::ifstream file(FLAGS_fixed_location_file_path);
      if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
          /* Only support fix location format to make it simple.
           * Records will only contains 'Fix' prefix.
           * Sample line:
           * Fix,GPS,37.7925002,-122.3979132,13.462797,0.000000,48.000000,0.000000,1593029872254,0.581968,0.000000
           * Sending at 1Hz, currently user should provide a fixed location
           * file that has one location per second. need some extra work to
           * make it more generic, i.e. align with the timestamp in the file.
           */
          {
            std::lock_guard<std::mutex> lock(cached_fixed_location_mutex);
            cached_fixed_location = line;
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
          file.close();
      } else {
        LOG(ERROR) << "Can not open fixed location file: "
                   << FLAGS_gnss_file_path;
        return;
      }
    }

    void ReadGnssRawMeasurement() {
      std::ifstream file(FLAGS_gnss_file_path);

      if (file.is_open()) {
        std::string line;
        std::string cached_line = "";
        std::string header = "";

        while (!cached_line.empty() || std::getline(file, line)) {
          if (!cached_line.empty()) {
            line = cached_line;
            cached_line = "";
          }

          // Get data header.
          if (header.empty() && android::base::StartsWith(line, "# Raw")) {
            header = line;
            LOG(DEBUG) << "Header: " << header;
            continue;
          }

          // Ignore not raw measurement data.
          if (!android::base::StartsWith(line, "Raw")) {
            continue;
          }

          {
            std::lock_guard<std::mutex> lock(cached_gnss_raw_mutex);
            cached_gnss_raw = header + "\n" + line;

            std::string new_line = "";
            while (std::getline(file, new_line)) {
              // Group raw data by TimeNanos.
              if (getTimeNanosFromLine(new_line) ==
                  getTimeNanosFromLine(line)) {
                cached_gnss_raw += "\n" + new_line;
              } else {
                cached_line = new_line;
                break;
              }
            }
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        file.close();
      } else {
        LOG(ERROR) << "Can not open GNSS Raw file: " << FLAGS_gnss_file_path;
        return;
      }
    }

    ~GnssGrpcProxyServiceImpl() {
      if (fixed_location_file_read_thread_.joinable()) {
        fixed_location_file_read_thread_.join();
      }
      if (fixed_location_write_thread_.joinable()) {
        fixed_location_write_thread_.join();
      }
      if (measurement_file_read_thread_.joinable()) {
        measurement_file_read_thread_.join();
      }
      if (measurement_read_thread_.joinable()) {
        measurement_read_thread_.join();
      }
      if (fixed_location_read_thread_.joinable()) {
        fixed_location_read_thread_.join();
      }
    }

  private:
  //不断的从source_out中读取
   void SendCommand(std::string command, cuttlefish::SharedFD source_out,
                    int out_fd) {
     std::vector<char> buffer(GNSS_SERIAL_BUFFER_SIZE);
     std::string cmd_str;
     auto bytes_read = source_out->Read(buffer.data(), buffer.size());
     if (bytes_read > 0) {
       std::string s(buffer.data(), bytes_read);
       cmd_str += s;
       // In case random string sent though /dev/gnss1, cmd_str will
       // auto resize, to get rid of first page.
       if (cmd_str.size() > GNSS_SERIAL_BUFFER_SIZE * 2) {
         cmd_str = cmd_str.substr(cmd_str.size() - GNSS_SERIAL_BUFFER_SIZE);
       }
       if (cmd_str.find(command) != std::string::npos) {
         if (command == CMD_GET_RAWMEASUREMENT) {
           sendGnssRawToSerial();
         } else if (command == CMD_GET_LOCATION) {
           sendToSerial();
         }
         cmd_str = "";
       }
     } else {
       if (source_out->GetErrno() == EAGAIN ||
           source_out->GetErrno() == EWOULDBLOCK) {
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
       } else {
         LOG(ERROR) << "Error reading fd " << out_fd << ": "
                    << " Error code: " << source_out->GetErrno()
                    << " Error sg:" << source_out->StrError();
       }
     }
   }

   [[noreturn]] void ReadMeasurementLoop() {
     int flags = gnss_out_->Fcntl(F_GETFL, 0);
     gnss_out_->Fcntl(F_SETFL, flags | O_NONBLOCK);

     while (true) {
       SendCommand(CMD_GET_RAWMEASUREMENT, gnss_out_, FLAGS_gnss_out_fd);
     }
   }

   [[noreturn]] void ReadFixedLocLoop() {
     int flags2 = fixed_location_out_->Fcntl(F_GETFL, 0);
     fixed_location_out_->Fcntl(F_SETFL, flags2 | O_NONBLOCK);
     while (true) {
       SendCommand(CMD_GET_LOCATION, fixed_location_out_,
                   FLAGS_fixed_location_out_fd);
     }
   }

   [[noreturn]] void WriteFixedLocationFromQueue() {
      while (true) {
         if (!fixed_locations_queue_.empty()) {
         std::string dataPoint = fixed_locations_queue_.front();
         std::string line = GenerateGpsLine(dataPoint);
         std::lock_guard<std::mutex> lock(cached_fixed_location_mutex);
         cached_fixed_location = line;
         {
           std::lock_guard<std::mutex> lock(fixed_locations_queue_mutex_);
           fixed_locations_queue_.pop();
         }
       }
       std::this_thread::sleep_for(std::chrono::milliseconds(fixed_locations_delay_));
     }
   }

    //第二个元素是时间
    std::string getTimeNanosFromLine(const std::string& line) {
      // TimeNanos is in column #3.
      std::vector<std::string> vals = android::base::Split(line, ",");
      return vals.size() >= 3 ? vals[2] : "-1";
    }

    //是否是RAW数据
    bool isGnssRawMeasurement(const std::string& inputStr) {
      // TODO: add more logic check to by pass invalid data.
      return !inputStr.empty() && android::base::StartsWith(inputStr, "# Raw");
    }

    cuttlefish::SharedFD gnss_in_;
    cuttlefish::SharedFD gnss_out_;
    cuttlefish::SharedFD fixed_location_in_;
    cuttlefish::SharedFD fixed_location_out_;

    std::thread measurement_read_thread_;
    std::thread fixed_location_read_thread_;
    std::thread fixed_location_file_read_thread_;
    std::thread fixed_location_write_thread_;
    std::thread measurement_file_read_thread_;

    std::string cached_fixed_location;
    std::mutex cached_fixed_location_mutex;

    std::string cached_gnss_raw;
    std::string previous_cached_gnss_raw;
    std::mutex cached_gnss_raw_mutex;

    std::queue<std::string> fixed_locations_queue_;
    std::mutex fixed_locations_queue_mutex_;
    int fixed_locations_delay_;
};

void RunServer() {
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  auto gnss_in = cuttlefish::SharedFD::Dup(FLAGS_gnss_in_fd);
  close(FLAGS_gnss_in_fd);
  if (!gnss_in->IsOpen()) {
    LOG(ERROR) << "Error dupping fd " << FLAGS_gnss_in_fd << ": "
               << gnss_in->StrError();
    return;
  }
  close(FLAGS_gnss_in_fd);

  auto gnss_out = cuttlefish::SharedFD::Dup(FLAGS_gnss_out_fd);
  close(FLAGS_gnss_out_fd);
  if (!gnss_out->IsOpen()) {
    LOG(ERROR) << "Error dupping fd " << FLAGS_gnss_out_fd << ": "
               << gnss_out->StrError();
    return;
  }

  auto fixed_location_in =
      cuttlefish::SharedFD::Dup(FLAGS_fixed_location_in_fd);
  close(FLAGS_fixed_location_in_fd);
  if (!fixed_location_in->IsOpen()) {
    LOG(ERROR) << "Error dupping fd " << FLAGS_fixed_location_in_fd << ": "
               << fixed_location_in->StrError();
    return;
  }
  close(FLAGS_fixed_location_in_fd);

  auto fixed_location_out =
      cuttlefish::SharedFD::Dup(FLAGS_fixed_location_out_fd);
  close(FLAGS_fixed_location_out_fd);
  if (!fixed_location_out->IsOpen()) {
    LOG(ERROR) << "Error dupping fd " << FLAGS_fixed_location_out_fd << ": "
               << fixed_location_out->StrError();
    return;
  }

  auto server_address("0.0.0.0:" + std::to_string(FLAGS_gnss_grpc_port));
  GnssGrpcProxyServiceImpl service(gnss_in, gnss_out, fixed_location_in,
                                   fixed_location_out);
  service.StartServer();
  if (!FLAGS_gnss_file_path.empty()) {
    // TODO: On-demand start the read file threads according to data type.
    service.StartReadFixedLocationFileThread();
    service.StartReadGnssRawMeasurementFileThread();

    // In the local mode, we are not start a grpc server, use a infinite loop instead
    while(true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
  } else {
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    if (!FLAGS_gnss_grpc_socket.empty()) {
      builder.AddListeningPort("unix:" + FLAGS_gnss_grpc_socket,
                               grpc::InsecureServerCredentials());
    }
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
  }

}


int main(int argc, char** argv) {
  cuttlefish::DefaultSubprocessLogging(argv);
  ::gflags::ParseCommandLineFlags(&argc, &argv, true);

  LOG(DEBUG) << "Starting gnss grpc proxy server...";
  RunServer();

  return 0;
}





