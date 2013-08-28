//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef __SERVICECONFIG_H__
#define __SERVICECONFIG_H__

#include <omnetpp.h>
#include <vector>
#include <string>
#include <algorithm>

/**
 * The Service Item currently holds only name and priority.
 * Generally it should have all the attributes/parameters that
 * characterize this service.
 */
typedef struct _srv_item{
	std::string name;
	double priority;
	uint16_t vlan;
	double msr;
	double mrr;
	friend std::ostream & operator<<(std::ostream & out, const  struct _srv_item &sit);
	bool operator< (const struct _srv_item & b) const{
		EV<<"Called"<<endl;
		return (priority > b.priority);}
} SrvItem;

// A vector of the available Service Items (SrvItem)
typedef std::vector<SrvItem> SrvList;

/**
 * A simple module to configure all the services in a scenario
 * and their parameters. This module is accessible from any other
 * module that needs this information, so we avoid declaring the
 * same thing again and again.
 *
 * Usually it is going to be accessed by the queue modules that need
 * to take under consideration the service names and priorities. If
 * there is no such module, the default values are going to be applied.
 *
 * 15-Feb-2011: Keep the services sorted by priority
 */
class ServiceConfig : public cSimpleModule
{
  public:
	SrvList srvs;
	ServiceConfig();

	/**
	 * Return the index of the service with the minimum priority.
	 */
	virtual int getLowPriorityService();
	/**
	 * Return the index of the service with the maximum priority.
	 */
	virtual int getHighPriorityService();


  protected:
	/**
	 * Read the configuration (parameters from the .ini file) and
	 * initialize the srvs members.
	 */
    virtual void initialize();

    /**
     * Currently dummy...
     */
    virtual void handleMessage(cMessage *msg);


};

#endif
