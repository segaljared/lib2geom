#ifndef __2GEOM_REGION_H
#define __2GEOM_REGION_H

#include "path.h"
#include "path-intersection.h"


//for path_direction:
#include "sbasis-geometric.h"

namespace Geom {

//returns true for ccw, false for cw
inline bool path_direction(Path const &p) {
    Piecewise<D2<SBasis> > pw = p.toPwSb();
    double area;
    Point centre;
    Geom::centroid(pw, centre, area);
    return area > 0;
}

class Region {
    friend std::vector<Region> region_boolean(bool rev, Region const &, Region const &,
                                              Crossings const &, Crossings const &);
    Path _boundary;
    boost::optional<Rect> box;
    bool _fill;
  public:

    Region() {}
    Region(Path const &p) : _boundary(p) { _fill = path_direction(p); }
    Region(Path const &p, bool dir) : _fill(dir), _boundary(p) {}
    Region(Path const &p, boost::optional<Rect> const &b) : _boundary(p), box(b) { _fill = path_direction(p); }
    Region(Path const &p, boost::optional<Rect> const &b, bool dir) : _fill(dir), _boundary(p), box(b) {}
    
    bool fill() const { return _fill; }    
    Path boundary() const { return _boundary; }
    Rect boundsFast() {
        if(!box) box = boost::optional<Rect>(boundary().boundsFast());
        return *box;
    }
    bool contains(Point const &p) const {
        return Geom::contains(boundary(), p);
    }
    
    Region inverse() const { return Region(_boundary.reverse(), box, !_fill); }
    
    Region operator*(Matrix const &m) const;
};

typedef std::vector<Region> Regions;

unsigned outer_index(Regions const &ps);

//assumes they're already sanitized somewhat
inline Regions regions_from_paths(std::vector<Path> const &ps) {
    Regions res;
    for(unsigned i = 0; i < ps.size(); i++) {
        res.push_back(Region(ps[i]));
    }
    return res;
}

Regions region_boolean(bool rev, Region const & a, Region const & b, Crossings const &cr);
Regions region_boolean(bool rev, Region const & a, Region const & b, Crossings const & cr_a, Crossings const & cr_b);

inline Regions region_boolean(bool rev, Region const & a, Region const & b) {
    return region_boolean(rev, a, b, crossings(a.boundary(), b.boundary()));
}

Regions path_union(Region const & a, Region const & b, bool typ);
Regions path_subtract(Region const & a, Region const & b, bool typ);
Regions path_intersect(Region const & a, Region const & b, bool typ);
Regions path_exclude(Region const & a, Region const & b, bool typ);

}

#endif