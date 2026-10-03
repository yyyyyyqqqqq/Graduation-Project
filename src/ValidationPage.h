#pragma once
#include "ValidationExperiment.h"
#include <QWidget>
class QTextBrowser;
class QComboBox;
class ValidationPage final : public QWidget
{
    Q_OBJECT
  public:
    explicit ValidationPage(QWidget *parent = nullptr, const QString &directory = ":/validation");

  private:
    void showEvent(QShowEvent *event) override;
    void showSample();
    QString m_directory;
    bool m_started = false;
    QTextBrowser *m_summary, *m_detail;
    QComboBox *m_samples, *m_threshold;
    std::optional<Validation::Dataset> m_dataset;
    Validation::Experiment m_experiment;
};
