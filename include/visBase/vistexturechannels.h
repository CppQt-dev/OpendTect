#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Jun 2008
________________________________________________________________________


-*/

#include "visdata.h"
#include "odmemory.h"

class SbImagei32;
namespace osgGeo { class LayeredTexture; }

namespace visBase
{

class MappedTextureDataSet;
class TextureChannel2RGBA;
class ChannelInfo;

/*!
Base class to manage the set of interchangeable and overlayable data layers
from which the texture displayed on some kind of section will be assembled
dynamically.

Different attributes are stored in different channels. Each channel may have
multiple versions, for example obtained with different parameter settings of
the attribute.

Each channel may consist of different components. Each component corresponds
with one data layer. Multiple components are not yet used.

Every component may utilize up to four bands in the image that stores the data.
The first band contains the data values itself. Additional bands are used to
to improve the quality of the displayed texture. One band is used to denote
undefined values separate from the actual signal. Other band(s) for example
store the instantaneous power of the signal.
*/

mExpClass(visBase) TextureChannels : public DataObject
{
    class TextureCallbackHandler;

public:
    static TextureChannels*	create()
				mCreateDataObj(TextureChannels);

    virtual bool		turnOn(bool yn);
    virtual bool		isOn() const;

    int				nrChannels() const;
    int				addChannel();
    int				insertChannel(int);
    void			removeChannel(int);
    void			swapChannels(int,int);

    void			setColTabMapper(int ch,const ColTab::Mapper&);
    const ColTab::Mapper&	getColTabMapper(int channel) const;
    void			reMapData(int,const TaskRunnerProvider&);

    void			setSize(int channel,int sz0,int sz1,int sz2);
    int				getSize(int channel,unsigned char dim) const;

    void			setOrigin(int channel,const Coord& origin);
    Coord			getOrigin(int channel) const;
    void			setScale(int channel,const Coord& scale);
    Coord			getScale(int channel) const;

    StepInterval<float>		getEnvelopeRange(unsigned char dim) const;

    void			setNrComponents(int channel,int nrcomp);
    int				getNrComponents(int channel) const;

    void			setNrVersions(int channel,int nrvers);
    int				nrVersions(int channel) const;
    int				currentVersion(int channel) const;
    void			setCurrentVersion(int channel,int version);

    bool			isCurrentDataPremapped(int channel) const;

    bool			setUnMappedVSData(int channel,int version,
				    const ValueSeries<float>*,OD::PtrPolicy,
				    const TaskRunnerProvider&);
    bool			setUnMappedData(int channel,int version,
				    const float*,OD::PtrPolicy,
				    const TaskRunnerProvider&);
    bool			setMappedData(int channel,int version,
					      unsigned char*,OD::PtrPolicy);
    void			unfreezeOldData(int channel);

    bool			setChannels2RGBA(TextureChannel2RGBA*);
    TextureChannel2RGBA*	getChannels2RGBA();
    const TextureChannel2RGBA*	getChannels2RGBA() const;

    const SbImagei32*		getChannels() const;

    osgGeo::LayeredTexture*	getOsgTexture() { return osgtexture_; }
    const osgGeo::LayeredTexture* getOsgTexture() const { return osgtexture_; }
    const TypeSet<int>*		getOsgIDs(int channel) const;

    void			enableTextureInterpolation(bool);
    bool			textureInterpolationEnabled() const;

    void			setNonShaderResolution(int);
    int			getNonShaderResolution() const;

    unsigned char		nrDataBands() const;
    inline unsigned char	nrUdfBands() const	   { return 1; }
    inline unsigned char	nrTextureBands() const
				{ return nrDataBands() + nrUdfBands(); }


protected:
    friend			class ChannelInfo;
    void			update(int channel,bool freezeifnodata=true);
    void			update(ChannelInfo*);
				~TextureChannels();

    TextureCallbackHandler*	texturecallbackhandler_;
    ObjectSet<ChannelInfo>	channelinfo_;

    TextureChannel2RGBA*	tc2rgba_;
    osgGeo::LayeredTexture*	osgtexture_;
    bool			interpolatetexture_;
};


}; // Namespace
