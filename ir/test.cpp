#include "IR.h"
#include "Coord.h"

#include <fstream>

int main() {

    Printer::init(2);
    std::ofstream file;

    {
        ir::Identifier h("H");

        ir::Identifier Vx("Vx");
        ir::Identifier Vy("Vy");
        ir::Identifier Vz("Vz");
        ir::VectExpr v(Vx, Vy, Vz);

        std::cout << "#nodes: after var init: " << ir::Node::getNodeNumber() << "\n";
#if 1
        {
            CartesianCoord cartesian;

            ir::VectExpr gradH = cartesian.grad(h);
            gradH.display("grad in cartesian coordinates");

            ir::BinExpr divV = cartesian.div(v);
            divV.display("div in cartesian coordinates");

            ir::VectExpr curlV = cartesian.curl(v);
            curlV.display("curl in cartesian coordinates");

            ir::BinExpr lapH = cartesian.div(cartesian.grad(h));
            // ir::Equation poisson("poisson", lapH, ir::Value<float>(0), NULL);
        }
        std::cout << "remaining nodes (after cartesian coord): "
            << ir::Node::getNodeNumber() << "\n";
#endif

#if 1
        {
            SphericalCoord spherical;

            ir::VectExpr gradH = spherical.grad(h);
            gradH.display("grad in spherical coordinates");

            ir::BinExpr divV = spherical.div(v);
            divV.display("div in spherical coordinates");

            ir::VectExpr curlV = spherical.curl(v);
            curlV.display("curl in spherical coordinates");
        }
        std::cout << "remaining nodes (after spherical coord): "
            << ir::Node::getNodeNumber() << "\n";
#endif

#if 0
        SpheroidalCoord spheroidal;

        gradH = spheroidal.grad(h);
        gradH->display("grad in spherical coordinates");
        delete gradH;

        divV = spheroidal.div(v);
        divV->display("div in spherical coordinates");

        // curlV = spheroidal.curl(v);
        // curlV->display("curl in cartesian coordinates");
#endif
    }
    std::cout << "remaining nodes: " << ir::Node::getNodeNumber() << "\n";
    return 0;
}
