#
#  Filename : nature.py
#  Author   : Stephane Grabli
#  Date     : 04/08/2005
#  Purpose  : Uses the NatureUP1D predicate to select the lines
#             of a given type (among SILHOUETTE, CREASE, SUGGESTIVE_CONTOURS,
#             BORDERS).
#             The suggestive contours must have been enabled in the 
#             options dialog to appear in the View Map.
#
#############################################################################  
#
#  Copyright (C) : Please refer to the COPYRIGHT file distributed 
#  with this source distribution. 
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
#############################################################################

from Freestyle import *
from logical_operators import *
from PredicatesB1D import *
from shaders import *

Operators.select(AndUP1D(pyNatureUP1D(SURFACE_INTERSECTION),QuantitativeInvisibilityUP1D(0)))
Operators.bidirectionalChain(ChainSilhouetteIterator(),NotUP1D( pyNatureUP1D( SURFACE_INTERSECTION) ) )
shaders_list = 	[
		ConstantColorShader(0,.5,0),
                pyConstantThicknessShader(4),
		]
Operators.create(TrueUP1D(), shaders_list)
