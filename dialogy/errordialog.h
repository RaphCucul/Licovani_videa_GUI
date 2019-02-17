#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#include <QDialog>
#include <QString>
#include <QWidget>
#include <QLabel>
#include <QGraphicsColorizeEffect>
#include <QGraphicsDropShadowEffect>
#include <QSequentialAnimationGroup>

class ErrorDialog : public QObject
{
    Q_OBJECT

public:
    enum class ErrorType{
        INFO,
        SOFT_ERROR,
        HARD_ERROR,
        WHAT_TODO
    };
    explicit ErrorDialog(QWidget* widgetWithNotification,QWidget *parent = nullptr);
    ~ErrorDialog();

    /**
     * @brief Function adds hover event to the error label - if user hover over the label, signal
     * is emitted and the error message is shown
     * @param obj = error QLabel
     * @param event - mouse hover
     * @return
     */
    bool eventFilter(QObject *obj, QEvent *event);

    /**
     * @brief Function analyse the position of the widget where error label will be mapped to
     * @param position - left, center or right
     */
    void evaluatePosition(QString position);

    /**
     * @brief The parent of the widget with error label is determined
     */
    void analyseParents();

    /**
     * @brief Function creates the error label based on the type of error
     * @param errorType - see ErrorType structure
     */
    void setErrorType(ErrorDialog::ErrorType errorType);

    /**
     * @brief The main function where all parameters of the error are set. Everything else
     * is hidden in this function, therefore user needs to set only 3 parameters and then call
     * show() function
     * @param position - where to place the error label
     * @param EventType
     * @param errorNumber - errors.h
     */
    void evaluate(QString position,QString EventType,int errorNumber);
    bool isEvaluated();

    /// initialization functions
    void initialiseErrorLabel();
    void Info();
    void SoftError();
    void HardError();
    void What_todo();
    void initShadowEffect();
    void initColorEffect();
    void initAnimation();
    void show();
    void showMessage(const QString errorMessage);
    void hide();

signals:
    void mouseHovered(QLabel*);

private:
    QString errorStatementToDisplay;
    int errorSeverity;
    QLabel* errorLabel = nullptr;
    QGraphicsColorizeEffect* highlightingEffect = nullptr;
    QGraphicsDropShadowEffect* glowEffect = nullptr;
    QSequentialAnimationGroup* animation = nullptr;
    QWidget* parentOfTheWidget = nullptr;
    QWidget* widgetWithError = nullptr;
    QWidget* errorLabelParent = nullptr;
    ErrorType errorType = ErrorType::INFO;
    bool errorLabelinitialized = false;
};

#endif // ERRORDIALOG_H
