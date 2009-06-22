#ifndef uiodfaulttreeitem_h
#define uiodfaulttreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodfaulttreeitem.h,v 1.11 2009-06-22 14:22:05 cvsjaap Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

#include "emposid.h"


namespace visSurvey { class FaultDisplay; class FaultStickSetDisplay; }


mDefineItem( FaultParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton );


mClass uiODFaultTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODFaultParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


mClass uiODFaultTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODFaultTreeItem( int, bool dummy );
    			uiODFaultTreeItem( const EM::ObjectID& emid );
    			~uiODFaultTreeItem();

protected:
    bool		askContinueAndSaveIfNeeded();
    void		prepareForShutdown();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    bool		init();
    const char*		parentType() const
			{return typeid(uiODFaultParentTreeItem).name();}

    EM::ObjectID		emid_;
    MenuItem			savemnuitem_;
    MenuItem			saveasmnuitem_;
    MenuItem			displaymnuitem_;
    MenuItem			displayplanemnuitem_;
    MenuItem			displaystickmnuitem_;
    MenuItem			displayintersectionmnuitem_;
    MenuItem			singlecolmnuitem_;
    MenuItem			removeselectedmnuitem_;
    visSurvey::FaultDisplay*	faultdisplay_;
};


mDefineItem( FaultStickSetParent, TreeItem, TreeTop,mShowMenu mMenuOnAnyButton);


mClass uiODFaultStickSetTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODFaultStickSetParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


mClass uiODFaultStickSetTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODFaultStickSetTreeItem(int,bool dummy);
    			uiODFaultStickSetTreeItem(const EM::ObjectID&);
    			~uiODFaultStickSetTreeItem();

protected:
    bool		askContinueAndSaveIfNeeded();
    void		prepareForShutdown();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    bool		init();
    const char*		parentType() const
			{return typeid(uiODFaultStickSetParentTreeItem).name();}


    EM::ObjectID			emid_;
    MenuItem				onlyatsectmnuitem_;
    MenuItem				savemnuitem_;
    MenuItem				saveasmnuitem_;
    MenuItem				removeselectedmnuitem_;
    visSurvey::FaultStickSetDisplay*	faultsticksetdisplay_;
};


#endif
