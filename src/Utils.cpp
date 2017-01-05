/*
 *  Backend utils
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2016 EPAM Systems Inc.
 */

#include "Utils.hpp"

#include <vector>

using std::string;
using std::to_string;
using std::vector;

namespace XenBackend {

/*******************************************************************************
 * Public
 ******************************************************************************/

string Utils::logDomId(domid_t domId, int id)
{
	return string("Dom(" + to_string(domId) + "/" + to_string(id) + ")");
}

string Utils::logState(xenbus_state state)
{
	static const vector<string> strStates = {"Unknown", "Initializing",
											 "InitWait", "Initialized",
											 "Connected",
											 "Closing", "Closed",
											 "Reconfiguring", "Reconfigured"};

	if (state >= strStates.size() || state < 0)
	{
		return "Error!!!";
	}
	else
	{
		return "[" + strStates[state] + "]";
	}
}

}
