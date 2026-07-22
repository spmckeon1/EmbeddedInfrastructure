#pragma once
//
//  ei_web.h
//
//
//  Created by Stephen McKeon on 7/21/26.
//

#include <Arduino.h>
#include <ei_types.h>

class Web
{
public:
    bool downloadingFile() const { return _downloadingFile; }
    void setDownloadingFile(bool downloading) { _downloadingFile = downloading; }

private:
    bool _downloadingFile = false;
};

extern Web web;
