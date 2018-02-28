#pragma once

#include <fmt/format.h>
#include <boost/multi_array.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <highfive/H5File.hpp>
#include <mutex>
#include <uWS/uWS.h>
#include <proto/fileLoadRequest.pb.h>
#include <proto/regionReadRequest.pb.h>
#include "proto/regionReadResponse.pb.h"
#include "compression.h"
#include "ctpl.h"

#define MAX_SUBSETS 8
#define MAX_THREADS 4
typedef boost::multi_array<float, 3> Matrix3F;
typedef boost::multi_array<float, 2> Matrix2F;

struct Histogram {
    int N;
    float firstBinCenter, binWidth;
    std::vector<int> bins;
};

struct BandStats {
    Histogram histogram;
    float minVal;
    float maxVal;
    float mean;
    std::vector<float> percentiles;
    std::vector<float> percentileVals;
    int nanCount;
};

struct ImageInfo {
    std::string filename;
    int depth;
    int width;
    int height;
    std::map<int, BandStats> bandStats;
};

class Session {
public:
    boost::uuids::uuid uuid;
protected:
    Matrix3F currentChannelCache;
    Histogram currentBandHistogram;
    int currentChannel;
    std::unique_ptr<HighFive::File> file;
    std::vector<HighFive::DataSet> dataSets;
    ImageInfo imageInfo;
    std::mutex eventMutex;
    uWS::WebSocket<uWS::SERVER>* socket;
    std::string baseFolder;
    std::vector<char> binaryPayloadCache;
    std::vector<char> compressionBuffers[MAX_SUBSETS];
    std::vector<std::string> availableFileList;
    bool verboseLogging;
    Responses::RegionReadResponse regionReadResponse;
    ctpl::thread_pool threadPool;

public:
    Session(uWS::WebSocket<uWS::SERVER>* ws, boost::uuids::uuid uuid, std::string folder, bool verbose = false);
    void onRegionRead(const Requests::RegionReadRequest& regionReadRequest);
    void onFileLoad(const Requests::FileLoadRequest& fileLoadRequest);
    ~Session();

protected:
    void updateHistogram();
    bool loadFile(const std::string& filename, int defaultBand = -1);
    bool loadChannel(int channel);
    bool loadStats();
    std::vector<float> getZProfile(int x, int y);
    std::vector<float> readRegion(const Requests::RegionReadRequest& regionReadRequest, bool meanFilter = true);
    void sendEvent(std::string eventName, google::protobuf::MessageLite& message);
    void log(const std::string& logMessage);
};

