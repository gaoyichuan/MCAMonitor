#include "spectrumdata.h"

SpectrumData::SpectrumData() {
}

SpectrumData::SpectrumData(std::string filename) {
    this->readFromFile(filename);
}

void SpectrumData::readFromFile(std::string filename) {
    //    fs::path path(filename);
    QFileInfo fileinfo(QString::fromStdString(filename));
    if(!fileinfo.exists()) {
        throw "File " + filename + " not exists";
    }

    QFile f(QString::fromStdString(filename));
    if(!f.open(QIODevice::ReadOnly)) {
        throw "Open file " + filename + " failed";
    }

    std::string ext_str = fileinfo.suffix().toLower().toStdString();
    if(fileExtensions.find(ext_str) == fileExtensions.end()) {
        throw "Unknown file extension";
    }

    uint32_t ext = fileExtensions.at(ext_str);
    switch(ext) {
    case FMT_ORTEC_CHN: {
        // read from chn file
        struct OrtecChnHeader header;
        if(fileinfo.size() < sizeof(OrtecChnHeader)) {
            throw "Ortec file too small";
        }

        try {
            QDataStream ifs(&f);
            ifs.readRawData((char *)&header, sizeof(OrtecChnHeader));

            std::stringstream time_str;
            struct std::tm tm;

            time_str << header.date << header.time_hhmm << header.time_ss;
            time_str >> std::get_time(&tm, "%d%b%y%H%M%S");

            this->acqStart = std::mktime(&tm);
            this->liveTime = header.live_time * 0.02;
            this->realTime = header.real_time * 0.02;

            this->dev_name = "MCA";
            this->filename = fileinfo.fileName();

            this->data.clear();
            uint32_t d;
            for(uint64_t i = 0; i < header.dlength; i++) {
                ifs.readRawData((char *)&d, sizeof(uint32_t));
                this->data.push_back(d);
            }
        } catch(std::exception &e) {
            throw e;
        }
        break;
    } // case FMT_ORTEC_CHN
    default:
        throw "Unknown file extension";
    }
}
