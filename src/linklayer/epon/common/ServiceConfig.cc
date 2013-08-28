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

#include "ServiceConfig.h"

Define_Module(ServiceConfig);

using std::string;

std::ostream & operator<<(std::ostream & out, const  struct _srv_item &sit){
	out<<"Service Name="<<sit.name<<", VlanID="<<sit.vlan<<", Priority="<<sit.priority <<", MSR="<<sit.msr;
	return out;
}

ServiceConfig::ServiceConfig(){}

void ServiceConfig::initialize()
{
	// Get the parameters
	cStringTokenizer tok(par("services").stringValue());
	std::vector<string> services=tok.asVector();
	cStringTokenizer tok2(par("srvPrior").stringValue());
	std::vector<double> sp=tok2.asDoubleVector();
	cStringTokenizer tok3(par("vlanMap").stringValue());
	std::vector<int> vlanmap=tok3.asIntVector();

	cStringTokenizer tok4(par("msrPerFlow").stringValue());
	std::vector<double> msrs=tok4.asDoubleVector();
	cStringTokenizer tok5(par("mrrPerFlow").stringValue());
	std::vector<double> mrrs=tok5.asDoubleVector();


	if (sp.size() != services.size() || sp.size() != vlanmap.size() || sp.size() != msrs.size() ||sp.size() != mrrs.size() )
		error("ServiceConfig: You must configure a priority, a vlan  and an MSR-MRR for each service. \n - Vlan ID"
				" 0 means no vlan (native) \n - MSR -1 means unlimited service \n - MRR -1 means we don't care");

	// Initialize the map
	for (uint32_t i=0; i<sp.size(); i++){
		SrvItem sit;
		sit.name = services[i];
		sit.priority = sp[i];
		sit.vlan = vlanmap[i];
		sit.msr = msrs[i];
		sit.mrr = mrrs[i];
		srvs.push_back(sit);
	}

	char buf[50];
	sprintf(buf, "%d Services", (int)srvs.size());
    getDisplayString().setTagArg("t",0,buf);

    std::sort(srvs.begin(), srvs.end());
	WATCH_VECTOR(srvs);
}

void ServiceConfig::handleMessage(cMessage *msg)
{
	// IDEA: - Dynamic Service registration...
	// probably useless though...
	delete msg;
}

int ServiceConfig::getLowPriorityService(){
	double min_p=100;
	int min_idx=0;
	for (uint32_t i=1; i<srvs.size(); i++){
		if (srvs[i].priority < min_p){
			min_p   = srvs[i].priority;
			min_idx = i;
		}
	}

	return min_idx;
}

int ServiceConfig::getHighPriorityService(){
	double max_p=-1;
	int max_idx=0;
	for (uint32_t i=1; i<srvs.size(); i++){
		if (srvs[i].priority > max_p){
			max_p   = srvs[i].priority;
			max_idx = i;
		}
	}

	return max_idx;
}


