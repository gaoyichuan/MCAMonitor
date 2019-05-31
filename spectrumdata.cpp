#include "spectrumdata.h"

namespace fs = std::filesystem;

SpectrumData::SpectrumData() {
}

SpectrumData::SpectrumData(std::string filename) {
    this->readFromFile(filename);
}

void SpectrumData::readFromFile(std::string filename) {
    fs::path path(filename);
    if(!fs::exists(path)) {
        throw "File " + filename + " not exists";
    }
    std::string ext_str = path.extension();
    std::transform(ext_str.begin(), ext_str.end(), ext_str.begin(), ::tolower);
    if(fileExtensions.find(ext_str) == fileExtensions.end()) {
        throw "Unknown file extension";
    }

    uint32_t ext = fileExtensions.at(ext_str);
    switch(ext) {
    case FMT_ORTEC_CHN: {
        // read from chn file
        struct OrtecChnHeader header;
        if(fs::file_size(path) < sizeof(OrtecChnHeader)) {
            throw "Ortec file too small";
        }

        try {
            std::ifstream ifs(path, std::ios::binary);
            ifs.read((char *)&header, sizeof(OrtecChnHeader));

            std::stringstream time_str;
            struct std::tm tm;

            time_str << header.date << header.time_hhmm << header.time_ss;
            time_str >> std::get_time(&tm, "%d%b%y%H%M%S");

            this->acqStart = std::mktime(&tm);
            this->liveTime = header.live_time * 0.02;
            this->realTime = header.real_time * 0.02;

            this->dev_name = "MCA";
            this->file = path;

            this->data.clear();
            uint32_t d;
            for(uint64_t i = 0; i < header.dlength; i++) {
                ifs.read((char *)&d, sizeof(uint32_t));
                this->data.push_back(d);
            }

            ifs.close();
        } catch(std::exception &e) {
            throw e;
        }
        break;
    } // case FMT_ORTEC_CHN
    default:
        throw "Unknown file extension";
    }
}
