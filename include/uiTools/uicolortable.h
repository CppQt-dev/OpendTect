#ifndef uicolortable_h
#define uicolortable_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert/Nanne
 Date:          Aug 2007
 RCS:           $Id: uicolortable.h,v 1.3 2008-04-08 05:05:07 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiColorTableCanvas;
class uiComboBox;
class uiLineEdit;

namespace ColTab { class Sequence; }

class uiColorTable : public uiGroup
{
public:

			uiColorTable(uiParent*,ColTab::Sequence& colseq,
				     bool vertical);
			   //!< Editable
			uiColorTable(uiParent*,const char*,bool vertical);
			   //!< Display only
			~uiColorTable();

    const ColTab::Sequence&	colTabSeq() const	{ return coltabseq_;}
    ColTab::Sequence&		colTabSeq()    		{ return coltabseq_;}

    void		setEdits(const Interval<float>&);
    void		setTable(const char*,bool emitnotif=true);
    void		setTable(ColTab::Sequence&,bool emitnotif=true);
    void		setHistogram(const TypeSet<float>*);
    Interval<float>&	getInterval()			{ return coltabrg_; }

    void                setAutoScale( bool yn )		{ autoscale_ = yn; }
    bool                autoScale() const		{ return autoscale_; }
    void                setClipRate( float r )		{ cliprate_ = r; }
    float               getClipRate() const		{ return cliprate_; }
    void                setSymmetry( bool yn )		{ symmetry_ = yn; }
    bool                getSymmetry() const		{ return symmetry_; }
    void                setSymmidval( float val )	{ symmidval_ = val; }
    float               getSymmidval() const		{ return symmidval_; }

    Notifier<uiColorTable>	seqChanged;
    Notifier<uiColorTable>	scaleChanged;

protected:

    bool		autoscale_;
    float		cliprate_;
    bool		symmetry_;
    float		symmidval_;

    ColTab::Sequence&	coltabseq_;
    bool		vertical_;
    Interval<float> 	coltabrg_;

    TypeSet<float>	histogram_;
    uiColorTableCanvas*	canvas_;
    uiLineEdit*		minfld_;
    uiLineEdit*		maxfld_;
    uiComboBox*		selfld_;

    void		canvasClick(CallBacker*);
    void		tabSel(CallBacker*);
    void		tableAdded(CallBacker*);
    void		rangeEntered(CallBacker*);
    void		doEdit(CallBacker*);
    void		doFlip(CallBacker*);
    void		editClipRate(CallBacker*);
    void		makeSymmetrical(CallBacker*);
    void		colTabChgdCB(CallBacker*);
    void		colTabManChgd(CallBacker*);

    bool		isEditable() const	{ return maxfld_; }
    void		fillTabList();
};


#endif
