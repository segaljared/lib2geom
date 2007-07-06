/*
 * piecewise.h - Piecewise function class
 *
 * Copyright 2007 Michael Sloan <mgsloan@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, output to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 */

#ifndef SEEN_GEOM_PW_SB_H
#define SEEN_GEOM_PW_SB_H

#include "s-basis.h"
#include <vector>
#include <map>

#include "concepts.h"
#include <boost/concept_check.hpp>

namespace Geom {

template <typename T>
class Piecewise {
  BOOST_CLASS_REQUIRE(T, Geom, FragmentConcept);

  public:
    std::vector<double> cuts;
    std::vector<T> segs;
    //segs[i] stretches from cuts[i] to cuts[i+1].

    Piecewise() {}

    explicit Piecewise(const T &s) {
        push_cut(0.);
        push_seg(s);
        push_cut(1.);
    }

    typedef typename T::output_type output_type;

    explicit Piecewise(const output_type & v) {
        push_cut(0.);
        push_seg(T(v));
        push_cut(1.);
    }

    inline T operator[](unsigned i) const { return segs[i]; }
    inline T &operator[](unsigned i) { return segs[i]; }
    inline output_type operator()(double t) const { return valueAt(t); }
    inline output_type valueAt(double t) const {
        unsigned n = segN(t);
        return segs[n](segT(t, n));
    }
    //TODO: maybe it is not a good idea to have this?
    Piecewise<T> operator()(SBasis f);
    Piecewise<T> operator()(Piecewise<SBasis>f);

    inline unsigned size() const { return segs.size(); }
    inline bool empty() const { return segs.empty(); }

    /**Convenience/implementation hiding function to add segment/cut pairs.
     * Asserts that basic size and order invariants are correct
     */
    inline void push(const T &s, double to) {
        assert(cuts.size() - segs.size() == 1);
        push_seg(s);
        push_cut(to);
    }
    //Convenience/implementation hiding function to add cuts.
    inline void push_cut(double c) {
        assert(cuts.empty() || c > cuts.back()); 
        cuts.push_back(c);
    }
    //Convenience/implementation hiding function to add segments.
    inline void push_seg(const T &s) { segs.push_back(s); }

    /**Returns the segment index which corresponds to a 'global' piecewise time.
     * Also takes optional low/high parameters to expedite the search for the segment.
     */
    inline unsigned segN(double t, int low = 0, int high = -1) const {
        high = (high == -1) ? size() : high;
        if(t < cuts[0]) return 0;
        if(t >= cuts[size()]) return size() - 1;
        while(low < high) {
            int mid = (high + low) / 2; //Lets not plan on having huge (> INT_MAX / 2) cut sequences
            double mv = cuts[mid];
            if(mv < t) {
                if(t < cuts[mid + 1]) return mid; else low = mid + 1;
            } else if(t < mv) {
                if(cuts[mid - 1] < t) return mid - 1; else high = mid - 1;
            } else {
                return mid;
            }
        }
        return low;
    }

    /**Returns the time within a segment, given the 'global' piecewise time.
     * Also takes an optional index parameter which may be used for efficiency or to find the time on a
     * segment outside its range.  If it is left to its default, -1, it will call segN to find the index.
     */
    inline double segT(double t, int i = -1) const {
        if(i == -1) i = segN(t);
        return (t - cuts[i]) / (cuts[i+1] - cuts[i]);
    }

    inline double mapToDomain(double t, unsigned i) const {
        return (1-t)*cuts[i] + t*cuts[i+1]; //same as: t * (cuts[i+1] - cuts[i]) + cuts[i]
    }    

    //Offsets the piecewise domain
    inline void offsetDomain(double o) {
        if(o != 0)
            for(unsigned i = 0; i <= size(); i++)
                cuts[i] += o;
    }

    //Scales the domain of the function by a value. 0 will result in an empty Piecewise.
    inline void scaleDomain(double s) {
        assert(s > 0);
        if(s == 0) {
            cuts.clear(); segs.clear();
            return;
        }
        for(unsigned i = 0; i <= size(); i++)
            cuts[i] *= s;
    }

    //Retrieves the domain in interval form
    inline Interval domain() const { return Interval(cuts.front(), cuts.back()); }

    //Transforms the domain into another interval
    inline void setDomain(Interval dom) {
        if(empty()) return;
        if(dom.isEmpty()) {
            cuts.clear(); segs.clear();
            return;
        }
        double cf = cuts.front();
        double o = dom.min() - cf, s = dom.extent() / (cuts.back() - cf);
        for(unsigned i = 0; i <= size(); i++)
            cuts[i] = (cuts[i] - cf) * s + o;
    }

    //Concatenates this Piecewise function with another, offseting time of the other to match the end.
    inline void concat(const Piecewise<T> &other) {
        if(other.empty()) return;

        if(empty()) { 
            cuts = other.cuts; segs = other.segs;
            return;
        }

        segs.insert(segs.end(), other.segs.begin(), other.segs.end());
        double t = cuts.back() - other.cuts.front();        
        for(unsigned i = 0; i < other.size(); i++)
            push_cut(other.cuts[i + 1] + t);
    }

    //Like concat, but ensures continuity.
    inline void continuousConcat(const Piecewise<T> &other) {
        boost::function_requires<AddableConcept<typename T::output_type> >();
        if(other.empty()) return;
        typename T::output_type y = segs.back().at1() - other.segs.front().at0();

        if(empty()) {
            for(unsigned i = 0; i < other.size(); i++)
                push_seg(other[i] + y);
            cuts = other.cuts;
            return;
        }

        double t = cuts.back() - other.cuts.front();
        for(unsigned i = 0; i < other.size(); i++)
            push(other[i] + y, other.cuts[i + 1] + t);
    }

    //returns true if the Piecewise<T> meets some basic invariants.
    inline bool invariants() const {
        // segs between cuts
        if(!(segs.size() + 1 == cuts.size() || (segs.empty() && cuts.empty())))
            return false;
        // cuts in order
        for(unsigned i = 0; i < segs.size(); i++)
            if(cuts[i] >= cuts[i+1])
                return false;
        return true;
    }

};

template<typename T>
inline Interval bounds_fast(const Piecewise<T> &f) {
    boost::function_requires<FragmentConcept<T> >();

    if(f.empty()) return Interval(0);
    Interval ret(bounds_fast(f[0]));
    for(unsigned i = 1; i < f.size(); i++)
        ret.unionWith(bounds_fast(f[i])); 
    return ret;
}

template<typename T>
inline Interval bounds_exact(const Piecewise<T> &f) {
    boost::function_requires<FragmentConcept<T> >();

    if(f.empty()) return Interval(0);
    Interval ret(bounds_exact(f[0]));
    for(unsigned i = 1; i < f.size(); i++)
        ret.unionWith(bounds_exact(f[i])); 
    return ret;
}

template<typename T>
inline Interval bounds_local(const Piecewise<T> &f, const Interval &m) {
    boost::function_requires<FragmentConcept<T> >();

    if(f.empty()) return Interval(0);
    if(m.isEmpty()) return Interval(f(m.min()));

    unsigned fi = f.segN(m.min()), ti = f.segN(m.max());
    double ft = f.segT(m.min(), fi), tt = f.segT(m.max(), ti);

    if(fi == ti) return bounds_local(f[fi], Interval(ft, tt));

    Interval ret(bounds_local(f[fi], Interval(ft, 1.)));
    for(unsigned i = fi + 1; i < ti; i++)
        ret.unionWith(bounds_exact(f[i]));
    if(tt != 0.) ret.unionWith(bounds_local(f[ti], Interval(0., tt)));

    return ret;
}

//returns a portion of a piece of a Piecewise<T>, given the piece's index and a to/from time.
template<typename T>
T elem_portion(const Piecewise<T> &a, unsigned i, double from, double to) {
    assert(i < a.size());
    double rwidth = 1 / (a.cuts[i+1] - a.cuts[i]);
    return portion( a[i], (from - a.cuts[i]) * rwidth, (to - a.cuts[i]) * rwidth );
}

/**Piecewise<T> partition(const Piecewise<T> &pw, std::vector<double> const &c);
 * Further subdivides the Piecewise<T> such that there is a cut at every value in c.
 * Precondition: c sorted lower to higher.
 * 
 * //Given Piecewise<T> a and b:
 * Piecewise<T> ac = a.partition(b.cuts);
 * Piecewise<T> bc = b.partition(a.cuts);
 * //ac.cuts should be equivalent to bc.cuts
 */
template<typename T>
Piecewise<T> partition(const Piecewise<T> &pw, std::vector<double> const &c) {
    if(c.empty()) return Piecewise<T>(pw);

    Piecewise<T> ret = Piecewise<T>();
    ret.cuts.reserve(c.size() + pw.cuts.size());
    ret.segs.reserve(c.size() + pw.cuts.size() - 1);

    if(pw.empty()) {
        ret.cuts = c;
        for(unsigned i = 0; i < c.size() - 1; i++)
            ret.push_seg(T());
        return ret;
    }

    unsigned si = 0, ci = 0;     //Segment index, Cut index

    //if the cuts have something earlier than the Piecewise<T>, add portions of the first segment
    while(c[ci] < pw.cuts.front() && ci < c.size()) {
        bool isLast = (ci == c.size()-1 || c[ci + 1] >= pw.cuts.front());
        ret.push_cut(c[ci]);
        ret.push_seg( elem_portion(pw, 0, c[ci], isLast ? pw.cuts.front() : c[ci + 1]) );
        ci++;
    }

    ret.push_cut(pw.cuts[0]);
    double prev = pw.cuts[0];    //previous cut
    //Loop which handles cuts within the Piecewise<T> domain
    //Should have the cuts = segs + 1 invariant
    while(si < pw.size() && ci <= c.size()) {
        if(ci == c.size() && prev <= pw.cuts[si]) { //cuts exhausted, straight copy the rest
            ret.segs.insert(ret.segs.end(), pw.segs.begin() + si, pw.segs.end());
            ret.cuts.insert(ret.cuts.end(), pw.cuts.begin() + si + 1, pw.cuts.end());
            return ret;
        }else if(ci == c.size() || c[ci] >= pw.cuts[si + 1]) {  //no more cuts within this segment, finalize
            if(prev > pw.cuts[si]) {      //segment already has cuts, so portion is required
                ret.push_seg(portion(pw[si], pw.segT(prev, si), 1.0));
            } else {                     //plain copy is fine
                ret.push_seg(pw[si]);
            }
            ret.push_cut(pw.cuts[si + 1]);
            prev = pw.cuts[si + 1];
            si++;
        } else if(c[ci] == pw.cuts[si]){                  //coincident
            //Already finalized the seg with the code immediately above
            ci++;
        } else {                                         //plain old subdivision
            ret.push(elem_portion(pw, si, prev, c[ci]), c[ci]);
            prev = c[ci];
            ci++;
        }
    }
    
    //input cuts extend further than this Piecewise<T>, extend the last segment.
    while(ci < c.size()) {
        if(c[ci] > prev) {
            ret.push(elem_portion(pw, pw.size() - 1, prev, c[ci]), c[ci]);
            prev = c[ci];
        }
        ci++;
    }
    return ret;
}

/**Piecewise<T> portion(const Piecewise<T> &pw, double from, double to);
 * Returns a Piecewise<T> with a defined domain of [min(from, to), max(from, to)].
 */
template<typename T>
Piecewise<T> portion(const Piecewise<T> &pw, double from, double to) {
    if(pw.empty() || from == to) return Piecewise<T>();

    Piecewise<T> ret;

    double temp = from;
    from = std::min(from, to);
    to = std::max(temp, to);
    
    unsigned i = pw.segN(from);
    ret.push_cut(from);
    if(i == pw.size() - 1 || to < pw.cuts[i + 1]) {    //to/from inhabit the same segment
        ret.push(elem_portion(pw, i, from, to), to);
        return ret;
    }
    ret.push_seg(portion( pw[i], pw.segT(from, i), 1.0 ));
    i++;
    unsigned fi = pw.segN(to, i);

    ret.segs.insert(ret.segs.end(), pw.segs.begin() + i, pw.segs.begin() + fi);  //copy segs
    ret.cuts.insert(ret.cuts.end(), pw.cuts.begin() + i, pw.cuts.begin() + fi + 1);  //and their cuts

    ret.push_seg( portion(pw[fi], 0.0, pw.segT(to, fi)));
    if(to != ret.cuts.back()) ret.push_cut(to);
    ret.invariants();
    return ret;
}

template<typename T>
Piecewise<T> remove_short_cuts(Piecewise<T> const &f, double tol) {
    Piecewise<T> ret;
    ret.push_cut(f.cuts[0]);
    for(unsigned i=0; i<f.size(); i++){
        if (f.cuts[i+1]-f.cuts[i] >= tol) {
            ret.push(f[i], f.cuts[i+1]);
        }
    }
    return ret;
}

template<typename T>
Piecewise<T> remove_short_cuts_extending(Piecewise<T> const &f, double tol) {
    Piecewise<T> ret;
    ret.push_cut(f.cuts[0]);
    double last = f.cuts[0]; // last cut included
    for(unsigned i=0; i<f.size(); i++){
        if (f.cuts[i+1]-f.cuts[i] >= tol) {
            ret.push(elem_portion(f, i, last, f.cuts[i+1]), f.cuts[i+1]);
            last = f.cuts[i+1];
        }
    }
    return ret;
}

template<typename T>
std::vector<double> roots(const Piecewise<T> &pw) {
    std::vector<double> ret;
    for(unsigned i = 0; i < pw.size(); i++) {
        std::vector<double> sr = roots(pw[i]);
        for (unsigned j = 0; j < sr.size(); j++) ret.push_back(sr[j] * (pw.cuts[i + 1] - pw.cuts[i]) + pw.cuts[i]);

    }
    return ret;
}

//IMPL: OffsetableConcept
template<typename T>
Piecewise<T> operator+(Piecewise<T> const &a, typename T::output_type b) {
    boost::function_requires<OffsetableConcept<T> >();
//TODO:empty
    Piecewise<T> ret = Piecewise<T>();
    ret.cuts = a.cuts;
    for(unsigned i = 0; i < a.size();i++)
        ret.push_seg(a[i] + b);
    return ret;
}
template<typename T>
Piecewise<T> operator-(Piecewise<T> const &a, typename T::output_type b) {
    boost::function_requires<OffsetableConcept<T> >();
//TODO: empty
    Piecewise<T> ret = Piecewise<T>();
    ret.cuts = a.cuts;
    for(unsigned i = 0; i < a.size();i++)
        ret.push_seg(a[i] - b);
    return ret;
}
template<typename T>
Piecewise<T> operator+=(Piecewise<T>& a, typename T::output_type b) {
    boost::function_requires<OffsetableConcept<T> >();

    if(a.empty()) { a.push_cut(0.); a.push(T(b), 1.); return a; }

    for(unsigned i = 0; i < a.size();i++)
        a[i] += b;
    return a;
}
template<typename T>
Piecewise<T> operator-=(Piecewise<T>& a, typename T::output_type b) {
    boost::function_requires<OffsetableConcept<T> >();

    if(a.empty()) { a.push_cut(0.); a.push(T(b), 1.); return a; }

    for(unsigned i = 0;i < a.size();i++)
        a[i] -= b;
    return a;
}

//IMPL: ScalableConcept
template<typename T>
Piecewise<T> operator-(Piecewise<T> const &a) {
    boost::function_requires<ScalableConcept<T> >();

    Piecewise<T> ret;
    ret.cuts = a.cuts;
    for(unsigned i = 0; i < a.size();i++)
        ret.push_seg(- a[i]);
    return ret;
}
template<typename T>
Piecewise<T> operator*(Piecewise<T> const &a, double b) {
    boost::function_requires<ScalableConcept<T> >();

    if(a.empty()) return Piecewise<T>();

    Piecewise<T> ret;
    ret.cuts = a.cuts;
    for(unsigned i = 0; i < a.size();i++)
        ret.push_seg(a[i] * b);
    return ret;
}
template<typename T>
Piecewise<T> operator/(Piecewise<T> const &a, double b) {
    boost::function_requires<ScalableConcept<T> >();

    //FIXME: b == 0?
    if(a.empty()) return Piecewise<T>();

    Piecewise<T> ret;
    ret.cuts = a.cuts;
    for(unsigned i = 0; i < a.size();i++)
        ret.push_seg(a[i] / b);
    return ret;
}
template<typename T>
Piecewise<T> operator*=(Piecewise<T>& a, double b) {
    boost::function_requires<ScalableConcept<T> >();

    if(a.empty()) return Piecewise<T>();

    for(unsigned i = 0; i < a.size();i++)
        a[i] *= b;
    return a;
}
template<typename T>
Piecewise<T> operator/=(Piecewise<T>& a, double b) {
    boost::function_requires<ScalableConcept<T> >();

    //FIXME: b == 0?
    if(a.empty()) return Piecewise<T>();

    for(unsigned i = 0; i < a.size();i++)
        a[i] /= b;
    return a;
}

//IMPL: AddableConcept
template<typename T>
Piecewise<T> operator+(Piecewise<T> const &a, Piecewise<T> const &b) {
    boost::function_requires<AddableConcept<T> >();

    Piecewise<T> pa = partition(a, b.cuts), pb = partition(b, a.cuts);
    Piecewise<T> ret = Piecewise<T>();
    assert(pa.size() == pb.size());
    ret.cuts = pa.cuts;
    for (unsigned i = 0; i < pa.size(); i++)
        ret.push_seg(pa[i] + pb[i]);
    return ret;
}
template<typename T>
Piecewise<T> operator-(Piecewise<T> const &a, Piecewise<T> const &b) {
    boost::function_requires<AddableConcept<T> >();

    Piecewise<T> pa = partition(a, b.cuts), pb = partition(b, a.cuts);
    Piecewise<T> ret = Piecewise<T>();
    assert(pa.size() == pb.size());
    ret.cuts = pa.cuts;
    for (unsigned i = 0; i < pa.size(); i++)
        ret.push_seg(pa[i] - pb[i]);
    return ret;
}
template<typename T>
inline Piecewise<T> operator+=(Piecewise<T> &a, Piecewise<T> const &b) {
    a = a+b;
    return a;
}
template<typename T>
inline Piecewise<T> operator-=(Piecewise<T> &a, Piecewise<T> const &b) {
    a = a-b;
    return a;
}

template<typename T1,typename T2>
Piecewise<T2> operator*(Piecewise<T1> const &a, Piecewise<T2> const &b) {
    //function_requires<MultiplicableConcept<T1> >();
    //function_requires<MultiplicableConcept<T2> >();

    Piecewise<T1> pa = partition(a, b.cuts);
    Piecewise<T2> pb = partition(b, a.cuts);
    Piecewise<T2> ret = Piecewise<T2>();
    assert(pa.size() == pb.size());
    ret.cuts = pa.cuts;
    for (unsigned i = 0; i < pa.size(); i++)
        ret.push_seg(pa[i] * pb[i]);
    return ret;
}

//TODO: operator/(pw, pw)

template<typename T>
inline Piecewise<T> operator*=(Piecewise<T> &a, Piecewise<T> const &b) {
    a = a * b;
    return a;
}

//TODO: operator/=(pw, pw)

Piecewise<SBasis> divide(Piecewise<SBasis> const &a, Piecewise<SBasis> const &b, unsigned k);
//TODO: replace divide(a,b,k) by divide(a,b,tol,k)?
//TODO: atm, relative error is ~(tol/a)%. Find a way to make it independant of a.
//Nota: the result is 'truncated' where b is smaller than 'zero': ~ a/max(b,zero). 
Piecewise<SBasis> 
divide(Piecewise<SBasis> const &a, Piecewise<SBasis> const &b, double tol, unsigned k, double zero=1.e-3);
Piecewise<SBasis> 
divide(SBasis const &a, Piecewise<SBasis> const &b, double tol, unsigned k, double zero=1.e-3);
Piecewise<SBasis> 
divide(Piecewise<SBasis> const &a, SBasis const &b, double tol, unsigned k, double zero=1.e-3);
Piecewise<SBasis> 
divide(SBasis const &a, SBasis const &b, double tol, unsigned k, double zero=1.e-3);

//Composition: functions called compose_foo are pieces of compose that are factored out in pw.cpp.
std::map<double,unsigned> compose_pullback(std::vector<double> const &cuts, SBasis const &g);
int compose_findSegIdx(std::map<double,unsigned>::iterator  const &cut,
                       std::map<double,unsigned>::iterator  const &next,
                       std::vector<double>  const &levels,
                       SBasis const &g);

//TODO: add concept check
template<typename T>
Piecewise<T> compose(Piecewise<T> const &f, SBasis const &g){
    Piecewise<T> result;
    if (f.empty()) return result;
    if (g.isZero()) return Piecewise<T>(f(0));
    if (f.size()==1){
        double t0 = f.cuts[0], width = f.cuts[1] - t0;
        return (Piecewise<T>) compose(f.segs[0],compose(Linear(-t0 / width, (1-t0) / width), g));
    }
    
    //first check bounds...
    Interval bs = bounds_fast(g);
    if (f.cuts.front() > bs.max()  || bs.min() > f.cuts.back()){
        int idx = (bs.max() < f.cuts[1]) ? 0 : f.cuts.size()-2;
        double t0 = f.cuts[idx], width = f.cuts[idx+1] - t0;
        return (Piecewise<T>) compose(f.segs[idx],compose(Linear(-t0 / width, (1-t0) / width), g));  
    }
    
    std::vector<double> levels;//we can forget first and last cuts...
    levels.insert(levels.begin(),f.cuts.begin()+1,f.cuts.end()-1);
    //TODO: use a std::vector<pairs<double,unsigned> > instead of a map<double,unsigned>.
    std::map<double,unsigned> cuts_pb = compose_pullback(levels,g);
    
    //-- Compose each piece of g with the relevant seg of f.
    result.cuts.push_back(0.);
    std::map<double,unsigned>::iterator cut=cuts_pb.begin();
    std::map<double,unsigned>::iterator next=cut; next++;
    while(next!=cuts_pb.end()){
        assert(std::abs(int((*cut).second-(*next).second))<1);
        //TODO: find a way to recover from this error? the root finder missed some root;
        //  the levels/variations of f might be too close/fast...
        int idx = compose_findSegIdx(cut,next,levels,g);
        double t0=(*cut).first;
        double t1=(*next).first;
        
        SBasis sub_g=compose(g, Linear(t0,t1));
        sub_g=compose(Linear(-f.cuts[idx]/(f.cuts[idx+1]-f.cuts[idx]),
                             (1-f.cuts[idx])/(f.cuts[idx+1]-f.cuts[idx])),sub_g);
        result.push(compose(f[idx],sub_g),t1);
        cut++;
        next++;
    }
    return(result);
} 

//TODO: add concept check
template<typename T>
Piecewise<T> compose(Piecewise<T> const &f, Piecewise<SBasis> const &g){
  Piecewise<T> result;
  for(unsigned i = 0; i < g.segs.size(); i++){
      Piecewise<T> fgi=compose(f, g.segs[i]);
      fgi.setDomain(Interval(g.cuts[i], g.cuts[i+1]));
      result.concat(fgi);
  }
  return result;
}

//TODO: add concept check
template <typename T>
Piecewise<T> Piecewise<T>::operator()(SBasis f){return compose((*this),f);}
//TODO: add concept check
template <typename T>
Piecewise<T> Piecewise<T>::operator()(Piecewise<SBasis>f){return compose((*this),f);}

//TODO: add concept check.
template<typename T>
Piecewise<T> integral(Piecewise<T> const &a) {
    Piecewise<T> result;
    result.segs.resize(a.segs.size());
    result.cuts = a.cuts;
    typename T::output_type c = a.segs[0].at0();
    for(unsigned i = 0; i < a.segs.size(); i++){
        result.segs[i] = integral(a.segs[i])*(a.cuts[i+1]-a.cuts[i]);
        result.segs[i]+= c-result.segs[i].at0();
        c = result.segs[i].at1();
    }
    return result;
}

//TODO: add concept check.
template<typename T>
Piecewise<T> derivative(Piecewise<T> const &a) {
    Piecewise<T> result;
    result.segs.resize(a.segs.size());
    result.cuts = a.cuts;
    for(unsigned i = 0; i < a.segs.size(); i++){
        result.segs[i] = derivative(a.segs[i])/(a.cuts[i+1]-a.cuts[i]);
    }
    return result;
}

std::vector<double> roots(Piecewise<SBasis> const &f);

}

#endif //SEEN_GEOM_PW_SB_H
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :