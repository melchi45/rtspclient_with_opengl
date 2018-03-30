/*******************************************************************************
*  Copyright (c) 1998 MFC Forum
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Module Name:
*
* Revision History:
*
* Date        Ver Name                    Description
* ----------  --- --------------------- -----------------------------------------
* 07-Jun-2016 0.1 Youngho Kim             Created
* ----------  --- --------------------- -----------------------------------------
*
* DESCRIPTION:
*
*  $Author:
*  $LastChangedBy:
*  $Date:
*  $Revision: 2949 $
*  $Id:
*  $HeadURL:
*******************************************************************************/
#ifndef MEDIA_BASICUSAGEENVIRONMENT_H_
#define MEDIA_BASICUSAGEENVIRONMENT_H_

#ifndef _BASIC_USAGE_ENVIRONMENT0_HH
#include <BasicUsageEnvironment0.hh>
#endif

class MediaBasicUsageEnvironment : public BasicUsageEnvironment0 {
protected:
	MediaBasicUsageEnvironment(TaskScheduler& taskScheduler);
	// called only by "createNew()" (or subclass constructors)
	virtual ~MediaBasicUsageEnvironment();

public:
	static MediaBasicUsageEnvironment* createNew(TaskScheduler& taskScheduler);

	// redefined virtual functions:
	virtual int getErrno() const;

	virtual UsageEnvironment& operator<<(char const* str);
	virtual UsageEnvironment& operator<<(int i);
	virtual UsageEnvironment& operator<<(unsigned u);
	virtual UsageEnvironment& operator<<(double d);
	virtual UsageEnvironment& operator<<(void* p);
};
#endif /* MEDIA_AMBABASICUSAGEENVIRONMENT_H_ */
