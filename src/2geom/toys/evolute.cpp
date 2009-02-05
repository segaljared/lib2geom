#include <2geom/basic-intersection.h>
#include <2geom/d2.h>
#include <2geom/sbasis.h>
#include <2geom/sbasis-2d.h>
#include <2geom/bezier-to-sbasis.h>

#include <2geom/toys/path-cairo.h>
#include <2geom/toys/toy-framework-2.h>

using std::vector;
using namespace Geom;

class SelfIntersect: public Toy {
    PointSetHandle psh;
virtual void draw(cairo_t *cr, std::ostringstream *notify, int width, int height, bool save, std::ostringstream *timer_stream) {
    cairo_set_line_width (cr, 0.5);
    cairo_set_source_rgba (cr, 0., 0., 0, 1);
    
    D2<SBasis> A(psh.asBezier());

    D2<SBasis> dA = derivative(A);
    D2<SBasis> ddA = derivative(dA);
    SBasis crs = cross(ddA, dA);
    cairo_d2_sb(cr, D2<SBasis>(Linear(0,1000), crs*(500./bounds_exact(crs)->extent())));
    vector<double> rts = roots(crs);
    for(int i = 0; i < rts.size(); i++) {
        draw_handle(cr, A(rts[i]));
    }
    cairo_d2_sb(cr, A);
    cairo_stroke(cr);
    Interval r(0, 1);
    if(rts.size() > 0)
        r[1] = rts[0];
    //if(rts[0] == 0)
    //rts.erase(rts.begin(), rts.begin()+1);
    A = portion(A, r[0], r[1]);
    dA = portion(dA, r[0], r[1]);
    ddA = portion(ddA, r[0], r[1]);
    crs = portion(crs, r[0], r[1]);
    cairo_stroke(cr);
    Piecewise<SBasis> s = divide(dA[0]*dot(dA,dA), crs, 100, 1);
    D2<Piecewise<SBasis> > ev4(Piecewise<SBasis>(A[0]) + divide(-dA[1]*dot(dA,dA), crs, 100, 1),
                               Piecewise<SBasis>(A[1]) + divide(dA[0]*dot(dA,dA), crs, 100, 1));
    cairo_d2_pw_sb(cr, ev4);
    cairo_stroke(cr);
    cairo_set_source_rgba (cr, 1., 0., 1, 1);
    
    Toy::draw(cr, notify, width, height, save,timer_stream);
}
public:
SelfIntersect (unsigned bez_ord) {
    handles.push_back(&psh);
    for(unsigned i = 0; i < bez_ord; i++)
        psh.push_back(uniform()*400, uniform()*400);
}
};

int main(int argc, char **argv) {   
    unsigned bez_ord=5;
    if(argc > 1)
        sscanf(argv[1], "%d", &bez_ord);
    init(argc, argv, new SelfIntersect(bez_ord));

    return 0;
}



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
