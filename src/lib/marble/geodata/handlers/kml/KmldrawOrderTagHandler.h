//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2012      Illya Kovalevskyy   <illya.kovalevskyy@gmail.com>
//

#ifndef KMLDRAWORDERTAGHANDLER_H
#define KMLDRAWORDERTAGHANDLER_H

#include "GeoTagHandler.h"

namespace Marble
{
namespace kml
{

class KmldrawOrderTagHandler : public GeoTagHandler
{
public:
    virtual GeoNode* parse(GeoParser &parser) const;
};

} // namespace kml
} // namespace Marble

#endif // KMLDRAWORDERTAGHANDLER_H
