#include "sculptorcontroller.h"

#include "mainwindow.h"
#include "meshconverter.h"
#include "operator.h"

SculptorController::SculptorController(MainWindow *mw) :
    sculptor(),
    mainWindow(mw),
    minToolRadius(0.01f),
    maxToolRadius(1.f)
{
    validSelection = false;

    sculptor.addOperator(new SweepOperator());
    sculptor.addOperator(new InfDefOperator());
    sculptor.addOperator(new TwistOperator());

    sculptor.setCurrentOperator(INFDEFLATE);

    sculptor.setRadius(minToolRadius);
    mainWindow->getOGLWidget()->getRenderer()->toolRadiusChanged(minToolRadius);
}

SculptorParameters SculptorController::getParameters() {
    return sculptor.getParameters();
}

void SculptorController::toolDialogHidden() {
    mainWindow->switchToolsVisibility(false);
}

void SculptorController::sweepSelected() {
    sculptor.setCurrentOperator(SWEEP);
    mainWindow->getToolsDialog()->setToolSelected(SWEEP);
}

void SculptorController::infDefSelected() {
    sculptor.setCurrentOperator(INFDEFLATE);
    mainWindow->getToolsDialog()->setToolSelected(INFDEFLATE);
}

void SculptorController::twistSelected() {
    sculptor.setCurrentOperator(TWIST);
    mainWindow->getToolsDialog()->setToolSelected(TWIST);
}

SculptorController::OperatorType SculptorController::getToolSelected() {
    return (SculptorController::OperatorType)(static_cast< SculptorController::OperatorType >(sculptor.getCurrentIndexOp()));
}

void SculptorController::mouseMoveEvent(QMouseEvent *e, int *selectionBuffer, bool found) {
    FtylRenderer *renderer = mainWindow->getOGLWidget()->getRenderer();

    if (found && e->modifiers() & Qt::ControlModifier) {
        FindSelectedVertex findSelectedVertex(selectionBuffer[1], selectionBuffer[2]);
        vortex::SceneGraph::PreOrderVisitor visitor(renderer->getScene()->sceneGraph(), findSelectedVertex);
        visitor.go();

        vertexSelected = findSelectedVertex.getVertexSelected();
        validSelection = true;

        //std::cout << "Vertex selected : (" << vertexSelected.mVertex.x << ", " << vertexSelected.mVertex.y << ", " << vertexSelected.mVertex.z << ")" << std::endl;

        renderer->setVertexSelected(vertexSelected.mVertex);
    } else {
        renderer->noSelection();
        validSelection = false;
    }
}

void SculptorController::mouseWheelEvent(QWheelEvent *e) {
    if (e->modifiers() & Qt::ControlModifier) {
        float nSteps = 100;
        float stepRadius = (maxToolRadius - minToolRadius) / nSteps;
        int sign = e->delta() > 0 ? 1 : -1;

        float newRadius = sculptor.getRadius() + sign*stepRadius;
        if (newRadius >= minToolRadius && newRadius <= maxToolRadius)
            toolRadiusChanged(newRadius);
    }
}

void SculptorController::mousePressEvent(QMouseEvent *e) {
    if (e->modifiers() & Qt::ControlModifier && sculptor.getCurrentIndexOp() != NO_OPERATOR) {
        vortex::Timer t;
        t.start();
        sculptor.loop(QuasiUniformMesh::Point(vertexSelected.mVertex.x, vertexSelected.mVertex.y, vertexSelected.mVertex.z));

        vortex::AssetManager *asset = mainWindow->getOGLWidget()->getRenderer()->getScene()->getAsset();
        vortex::Mesh *m = asset->getMesh(0);

        m->release();
        QuasiUniformMesh pm;
        sculptor.getMesh(pm);
        MeshConverter::convert(&pm, m);
        m->init();
        t.stop();
        std::cout << "Time : " << t.value() << std::endl;
    }
}

void SculptorController::mouseReleaseEvent(QMouseEvent *e) {

}

void SculptorController::sceneLoaded() {
    vortex::AssetManager *asset = mainWindow->getOGLWidget()->getRenderer()->getScene()->getAsset();

    vortex::Mesh *m = asset->getMesh(0);

    QuasiUniformMesh *pm = new QuasiUniformMesh(), pm2;

    MeshConverter::convert(m, pm);
    m->release();

    sculptor.setMesh(*pm);
    sculptor.getMesh(pm2);

    MeshConverter::convert(&pm2, m);
    m->init();

    mainWindow->getParametersDialog()->setParameters(sculptor.getParameters());
}

void SculptorController::toolRadiusChanged(float value) {
    assert(value >= minToolRadius && value <= maxToolRadius);
    sculptor.setRadius(value);
    mainWindow->getToolsDialog()->setToolRadius(value);
    mainWindow->getOGLWidget()->getRenderer()->toolRadiusChanged(value);
}

float SculptorController::getToolRadius() const {
    return sculptor.getRadius();
}

float SculptorController::getMinToolRadius() const {
    return minToolRadius;
}

float SculptorController::getMaxToolRadius() const {
    return maxToolRadius;
}

void FindSelectedVertex::operator()(vortex::SceneGraph::Node *theNode) {
    if (theNode->isLeaf()) {
        vortex::SceneGraph::LeafMeshNode *leafNode = static_cast<vortex::SceneGraph::LeafMeshNode *>(theNode);
        for (int i = 0; i < leafNode->nMeshes(); ++i) {
            if ((*leafNode)[i]) {
                vortex::Mesh::MeshPtr mesh = (*leafNode)[i];
                if (mesh->meshId() == meshSelectedId) {
                    vertexSelected = mesh->vertices()[mesh->indices()[faceSelectedId*3]];

                    // TODO : Project each vertex of the triangle on the screen and find closest to the pixel selected
                    /*float closestDist = FLT_MAX;
                    for (int i = 0 ; i <= 2; i++) {
                        vortex::Mesh::VertexData v = mesh->vertices()[mesh->indices()[faceSelectedId*3+i]];
                        if (dist(v.mVertex, ) < closestDist)
                            vertexSelected = v;
                    }*/
                }
            }
        }
    }
}

vortex::Mesh::VertexData FindSelectedVertex::getVertexSelected() const {
    return vertexSelected;
}

