/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AESinkFactory.h"
#include "Interfaces/AESink.h"
#if defined(TARGET_WINDOWS)
  #include "Sinks/AESinkWASAPI.h"
  #include "Sinks/AESinkDirectSound.h"
#elif defined(TARGET_ANDROID)
  #include "Sinks/AESinkAUDIOTRACK.h"
#elif defined(TARGET_RASPBERRY_PI)
  #include "Sinks/AESinkPi.h"
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
  #if defined(HAS_ALSA)
    #include "Sinks/AESinkALSA.h"
  #endif
  #if defined(HAS_PULSEAUDIO)
    #include "Sinks/AESinkPULSE.h"
  #endif
  #include "Sinks/AESinkOSS.h"
#else
  #pragma message("NOTICE: No audio sink for target platform.  Audio output will not be available.")
#endif
#include "Sinks/AESinkProfiler.h"
#include "Sinks/AESinkNULL.h"

#include "settings/AdvancedSettings.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

void CAESinkFactory::ParseDevice(std::string &device, std::string &driver)
{
  int pos = device.find_first_of(':');
  if (pos > 0)
  {
    driver = device.substr(0, pos);
    std::transform(driver.begin(), driver.end(), driver.begin(), ::toupper);

    // check that it is a valid driver name
    if (
#if defined(TARGET_WINDOWS)
        driver == "WASAPI"      ||
        driver == "DIRECTSOUND" ||
#elif defined(TARGET_ANDROID)
        driver == "AUDIOTRACK"  ||
#elif defined(TARGET_RASPBERRY_PI)
        driver == "PI"          ||
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
  #if defined(HAS_ALSA)
        driver == "ALSA"        ||
  #endif
  #if defined(HAS_PULSEAUDIO)
        driver == "PULSE"       ||
  #endif
        driver == "OSS"         ||
#endif
        driver == "PROFILER"    ||
        driver == "NULL")
      device = device.substr(pos + 1, device.length() - pos - 1);
    else
      driver.clear();
  }
  else
    driver.clear();
}

IAESink *CAESinkFactory::TrySink(std::string &driver, std::string &device, AEAudioFormat &format)
{
  IAESink *sink = NULL;

  if (driver == "NULL")
    sink = new CAESinkNULL();

#if defined(TARGET_WINDOWS)
  else if (driver == "WASAPI")
    sink = new CAESinkWASAPI();
  else if (driver == "DIRECTSOUND")
    sink = new CAESinkDirectSound();
#elif defined(TARGET_ANDROID)
  sink = new CAESinkAUDIOTRACK();
#elif defined(TARGET_RASPBERRY_PI)
  sink = new CAESinkPi();
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
  #if defined(HAS_PULSEAUDIO)
  else if (driver == "PULSE")
    sink = new CAESinkPULSE();
  #endif
  #if defined(HAS_ALSA)
  else if (driver == "ALSA")
    sink = new CAESinkALSA();
  #endif
  else if (driver == "OSS")
    sink = new CAESinkOSS();
#endif

  if (!sink)
    return NULL;

  if (sink->Initialize(format, device))
  {
    return sink;
  }
  sink->Deinitialize();
  delete sink;
  return NULL;
}

IAESink *CAESinkFactory::Create(std::string &device, AEAudioFormat &desiredFormat, bool rawPassthrough)
{
  // extract the driver from the device string if it exists
  std::string driver;
  ParseDevice(device, driver);

  AEAudioFormat  tmpFormat = desiredFormat;
  IAESink       *sink;
  std::string    tmpDevice = device;

  sink = TrySink(driver, tmpDevice, tmpFormat);
  if (sink)
  {
    desiredFormat = tmpFormat;
    return sink;
  }

  return NULL;
}

void CAESinkFactory::EnumerateEx(AESinkInfoList &list, bool force)
{
  AESinkInfo info;
#if defined(TARGET_WINDOWS)

  info.m_deviceInfoList.clear();
  info.m_sinkName = "DirectSound";
  CAESinkDirectSound::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);

  info.m_deviceInfoList.clear();
  info.m_sinkName = "WASAPI";
  CAESinkWASAPI::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);

#elif defined(TARGET_ANDROID)

  info.m_deviceInfoList.clear();
  info.m_sinkName = "AUDIOTRACK";
  CAESinkAUDIOTRACK::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);

#elif defined(TARGET_RASPBERRY_PI)

  info.m_deviceInfoList.clear();
  info.m_sinkName = "PI";
  CAESinkPi::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);

#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)

  #if defined(HAS_PULSEAUDIO)
  info.m_deviceInfoList.clear();
  info.m_sinkName = "PULSE";
  CAESinkPULSE::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
  {
    list.push_back(info);
    return;
  }
  #endif

  #if defined(HAS_ALSA)
  info.m_deviceInfoList.clear();
  info.m_sinkName = "ALSA";
  CAESinkALSA::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);
  #endif

  info.m_deviceInfoList.clear();
  info.m_sinkName = "OSS";
  CAESinkOSS::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);

#endif

}
