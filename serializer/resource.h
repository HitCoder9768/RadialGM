/**
* @file  resource.h
* @brief Header implementing a virtual base class for resource plugins.
*
* @section License
*
* Copyright (C) 2013 Robert B. Colton
* This file is a part of the RadialGM IDE.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
**/

namespace RGM {

#ifndef RESOURCE_H
#define RESOURCE_H

class Resource
{
public:
    virtual ~Resource();
    virtual void LoadDefaults() = 0;
    virtual void SaveDefaults() = 0;
    virtual void Initialize() = 0;
    virtual void Finalize() = 0;
};

#endif // RESOURCE_H

}
