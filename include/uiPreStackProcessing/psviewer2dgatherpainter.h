#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "position.h"
#include "datapack.h"

namespace FlatView { class Viewer; };
template <class T> class SamplingData;

class Gather;

//!Gather display

namespace PreStackView
{

mExpClass(uiPreStackProcessing) Viewer2DGatherPainter
{
public:
    				Viewer2DGatherPainter(FlatView::Viewer&);
    				~Viewer2DGatherPainter();

    void			setVDGather(DataPack::ID);
    void			setWVAGather(DataPack::ID);
    void			setColorTableClipRate(float);
    float			colorTableClipRate() const;

    BinID			getBinID() const;
    bool			hasData() const
    				{ return inputwvagather_ || inputvdgather_; }

protected:
    void			getGatherRange(const Gather*,
					       SamplingData<float>&,
					       int& nrsamp) const;

    FlatView::Viewer&			viewer_;
    ConstRefMan<Gather> inputwvagather_;
    ConstRefMan<Gather> inputvdgather_;
};


}; //namespace
