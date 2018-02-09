/*******************************************************************************
 *  Copyright (c) 2016 Hanwha Techwin Co., Ltd.
 *
 *  Licensed to the Hanwha Techwin Software Foundation under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  Smart Home Camera - Hanwha B2C Action Cam Project
 *  http://www.samsungsmartcam.com
 *
 *  Security Solution Division / Camera S/W Development Team
 *  Home Security Camera SW Dev. Group
 *
 *  @file
 *  Contains implementation of logging tools.
 *
 *  $Author: young_ho_kim $
 *  $LastChangedBy: young_ho_kim $
 *  $Date: 2016-08-02 21:06:33 +0900 (화, 02 8 2016) $
 *  $Revision: 2267 $
 *  $Id: AmbaBasicUsageEnvironment.cpp 2267 2016-08-02 12:06:33Z young_ho_kim $
 *  $HeadURL: http://ctf1.stw.net/svn/repos/wearable_camera/trunk/Source/Pixcam/Wear-1.0.5/app/live555_server/AmbaBasicUsageEnvironment.cpp $
 *******************************************************************************

  MODULE NAME:

  REVISION HISTORY:

  Date        Ver Name                    Description
  ----------  --- --------------------- -----------------------------------------
  07-Jun-2016 0.1 Youngho Kim             Created
 ...............................................................................

  DESCRIPTION:


 ...............................................................................
 *******************************************************************************/

#include "MediaBasicUsageEnvironment.h"
#include "log_utils.h"

////////// AmbaBasicUsageEnvironment //////////

#if defined(__WIN32__) || defined(_WIN32)
extern "C" int initializeWinsockIfNecessary();
#endif

MediaBasicUsageEnvironment*
MediaBasicUsageEnvironment::createNew(TaskScheduler& taskScheduler) {
	return new MediaBasicUsageEnvironment(taskScheduler);
}

MediaBasicUsageEnvironment::MediaBasicUsageEnvironment(TaskScheduler& taskScheduler)
: BasicUsageEnvironment0(taskScheduler) {
#if defined(__WIN32__) || defined(_WIN32)
  if (!initializeWinsockIfNecessary()) {
    setResultErrMsg("Failed to initialize 'winsock': ");
    reportBackgroundError();
    internalError();
  }
#endif
}

MediaBasicUsageEnvironment::~MediaBasicUsageEnvironment() {
}

int MediaBasicUsageEnvironment::getErrno() const {
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
  return WSAGetLastError();
#else
  return errno;
#endif
}

UsageEnvironment& MediaBasicUsageEnvironment::operator<<(char const* str) {
  if (str == NULL) str = "(NULL)"; // sanity check
  //fprintf(stderr, "%s", str);
  log_info("%s", str);
  return *this;
}

UsageEnvironment& MediaBasicUsageEnvironment::operator<<(int i) {
  //fprintf(stderr, "%d", i);
  log_info("%d", i);
  return *this;
}

UsageEnvironment& MediaBasicUsageEnvironment::operator<<(unsigned u) {
  //fprintf(stderr, "%u", u);
	log_info("%u", u);
  return *this;
}

UsageEnvironment& MediaBasicUsageEnvironment::operator<<(double d) {
  //fprintf(stderr, "%f", d);
	log_info("%f", d);
  return *this;
}

UsageEnvironment& MediaBasicUsageEnvironment::operator<<(void* p) {
  //fprintf(stderr, "%p", p);
  log_info("%p", p);
  return *this;
}
