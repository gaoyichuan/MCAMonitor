#ifndef SPECTRUMDATA_H
#define SPECTRUMDATA_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <QDataStream>
#include <QFile>
#include <QFileInfo>

#define FMT_ORTEC_CHN 1

static std::unordered_map<std::string, uint32_t> fileExtensions = {
    {"chn", FMT_ORTEC_CHN}};

struct OrtecChnHeader {
    // header
    uint16_t inop, dev_id, det_num;
    char time_ss[2];
    uint32_t real_time, live_time;
    char date[7], inopc, time_hhmm[4];
    uint16_t offset, dlength;
};

class SpectrumData {
  public:
    SpectrumData();
    SpectrumData(QString filename); // from file

    void readFromFile(QString filename);
    void saveToFile(QString filename);

    // private:
    time_t acqStart, acqStop;
    double realTime, liveTime; // in seconds
    std::string dev_name;
    QString filename;

    std::vector<uint64_t> data;
};

#endif // SPECTRUMDATA_H
