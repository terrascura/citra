#include <QDockWidget>
#include <QSlider>
#include "video_core/renderer_base.h"
class EmuThread;

class StereoscopicControllerWidget : public QDockWidget {
    Q_OBJECT

public:
    StereoscopicControllerWidget(QWidget* parent = nullptr);

public slots:
    void OnEmulationStarting(EmuThread* emu_thread);
    void OnEmulationStopping();

    void OnSliderMoved(int);
signals:
    void DepthChanged(float v);
    void StereoscopeModeChanged(RendererBase::StereoscopicMode mode);

private:
    QSlider* slider;
};
