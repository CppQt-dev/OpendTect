#ifndef curvature_h
#define curvature_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		May 2009
 RCS:		$Id: curvature.h,v 1.1 2009-05-15 15:17:09 cvskris Exp $
________________________________________________________________________

-*/

#include "general.h"

/*!Class to calculate curvature from 9 regularly sampled points. Equations are
   derived from Roberts 2001 in First Break. */

mClass Curvature
{
public:
    mClass Setup
    {
	public:
	    		Setup();

			mDefSetupMemb(bool,mean);
			mDefSetupMemb(bool,gaussian);
			mDefSetupMemb(bool,minmax);
			mDefSetupMemb(bool,mostposneg);
			mDefSetupMemb(bool,shapeindex );
			mDefSetupMemb(bool,dip );
			mDefSetupMemb(bool,strike);
			mDefSetupMemb(bool,contour );
			mDefSetupMemb(bool,curvedness );
    };

    			Curvature(const Setup&);

    		bool	set(double v00,double v01,double v02,
			    double v10,double v11,double v12,
			    double v20,double v21,double v22,
			    double dist01=1, double d10=1,
			    bool checkforudfs=false);
			/*!\param dist01 is the distance going from v00 to v01
			   \param dist10 is the distance going from v00 to v10
			   \returns true if no udf found */

    double	mean_;
    double	gaussian_;
    double	max_;
    double	min_;
    double	mostpos_;
    double	mostneg_;
    double	shapeindex_;
    double	dip_;
    double	strike_;
    double	contour_;
    double	curvedness_;

    Setup	setup_;
};


#endif
