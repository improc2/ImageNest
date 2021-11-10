/****************************************************************************
 * Copyright (C) 2009-2010 GGA Software Services LLC
 * 
 * This file is part of Imago toolkit.
 * 
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ***************************************************************************/

#include <cmath>
#include <vector>
#include <deque>
#include <stack>

#include "exception.h"
#include "fourier_descriptors.h"
#include "image.h"
#include "binarizer.h"
#include "png_saver.h"
#include "output.h"
#include "image_utils.h"
#include "algebra.h"
#include "contour_extractor.h"

using namespace imago;

void FourierDescriptors::calculate( const Segment *seg, int count,
                                    std::vector<double> &d)
{
   std::vector<Vec2d> contour;
   ContourExtractor().getApproxContour(*seg, contour);
   calculate(contour, count, d);
}

void FourierDescriptors::calculate( const Points &contour, int count,
                                    std::vector<double> &d)
{
   std::vector<double> &_desc = d;

   //PngSaver(*seg).saveImage("output/poly2.png");

   _desc.clear();

   double length = 0; //length of polygon
   double square;
   Vec2d p1, p2, p3;
   Vec2d v1, v2;

   for (int i = 1; i < (int)contour.size(); i++)
   {
      length += Vec2d::distance(contour[i - 1], contour[i]);
   }

   for (int i = 1; i <= count; i++)
   {
      double lk, phi;
      double a = 0, b = 0;

      for (int k = 1; k < (int)contour.size(); k++)
      {
         lk = 0;
         for (int j = 1; j <= k; j++)
         {
            lk += Vec2d::distance(contour[j - 1], contour[j]);
         }

         p1 = contour[k - 1];
         p2 = contour[k];

         if (k == (int)contour.size() - 1)
            p3 = contour[0];
         else
            p3 = contour[k + 1];

         v1.diff(p3, p2);
         v2.diff(p2, p1);

         phi = Vec2d::angle(v1, v2);

         square = p1.x * p2.y - p2.x * p1.y +
                  p2.x * p3.y - p3.x * p2.y +
                  p3.x * p1.y - p1.x * p3.y;

         if (square < 0)
            phi = -phi;

         a += sin(2 * PI * i * lk / length) * phi;
         b += cos(2 * PI * i * lk / length) * phi;
      }

      a /= -PI * i;
      b /= PI * i;

      //r = sqrt(a * a + b * b);

      _desc.push_back(a);
      _desc.push_back(b);
   }
}