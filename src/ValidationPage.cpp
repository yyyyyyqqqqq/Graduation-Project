#include "ValidationPage.h"
#include <QComboBox>
#include <QFutureWatcher>
#include <QLabel>
#include <QSplitter>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace
{
struct Loaded
{
    std::optional<Validation::Dataset> dataset;
    Validation::Experiment experiment;
    QString error;
};
} // namespace
ValidationPage::ValidationPage(QWidget *parent, const QString &directory)
    : QWidget(parent), m_directory(directory)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    auto *title = new QLabel(QStringLiteral("方法验证 / Rules v1"), this);
    title->setObjectName("pageTitle");
    layout->addWidget(title);
    auto *split = new QSplitter(Qt::Vertical, this);
    layout->addWidget(split, 1);
    m_summary = new QTextBrowser(split);
    m_summary->setObjectName("validationSummary");
    m_summary->setOpenExternalLinks(false);
    m_summary->setPlainText(QStringLiteral("正在校验冻结数据集并进行离线重放……"));
    auto *bottom = new QWidget(split);
    auto *detailLayout = new QVBoxLayout(bottom);
    detailLayout->addWidget(
        new QLabel(QStringLiteral("样本明细 / Sample details（★ 表示阈值敏感样本）"), bottom));
    m_samples = new QComboBox(bottom);
    m_samples->setObjectName("validationSamples");
    m_samples->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_samples->setMinimumContentsLength(20);
    detailLayout->addWidget(m_samples);
    m_threshold = new QComboBox(bottom);
    m_threshold->setObjectName("validationThreshold");
    m_threshold->addItems({"0.85 — Experimental", "0.90 — Production default", "0.95 — Experimental"});
    m_threshold->setCurrentIndex(1);
    detailLayout->addWidget(m_threshold);
    m_detail = new QTextBrowser(bottom);
    m_detail->setObjectName("validationDetail");
    m_detail->setOpenExternalLinks(false);
    detailLayout->addWidget(m_detail, 1);
    m_samples->setEnabled(false);
    m_threshold->setEnabled(false);
    split->setSizes({300, 300});
    connect(m_samples, &QComboBox::currentIndexChanged, this, [this] { showSample(); });
    connect(m_threshold, &QComboBox::currentIndexChanged, this, [this] { showSample(); });
}
void ValidationPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Do not start the offline experiment merely because the application opened another page.
    if (m_started)
        return;
    m_started = true;
    auto *watcher = new QFutureWatcher<Loaded>(this);
    connect(watcher, &QFutureWatcher<Loaded>::finished, this, [this, watcher] {
        auto loaded = watcher->result();
        watcher->deleteLater();
        if (!loaded.dataset)
        {
            m_summary->setPlainText(
                QStringLiteral("数据集校验失败；未执行实验。\nDataset contract rejected: ") + loaded.error);
            return;
        }
        m_dataset = std::move(loaded.dataset);
        m_experiment = std::move(loaded.experiment);
        m_summary->setPlainText(Validation::experimentSummary(*m_dataset, m_experiment));
        const auto sensitive = m_experiment.result["sensitiveSamples"].toArray();
        for (const auto &s : m_dataset->samples)
            m_samples->addItem((sensitive.contains(s.id) ? QStringLiteral("★ ") : QString()) +
                                   s.profile.key.snapshotIdentity.ecosystem + " / " +
                                   s.profile.key.snapshotIdentity.name + " / " + s.candidate.id(),
                               s.id);
        m_samples->setEnabled(true);
        m_threshold->setEnabled(true);
        showSample();
    });
    // The worker owns value-only data; closing the page cannot access destroyed widgets.
    watcher->setFuture(QtConcurrent::run([directory = m_directory] {
        Loaded r;
        r.dataset = Validation::loadRuntimeDirectory(directory, r.error);
        if (r.dataset)
            r.experiment = Validation::runExperiment(*r.dataset);
        return r;
    }));
}
void ValidationPage::showSample()
{
    const int i = m_samples->currentIndex();
    if (!m_dataset || i < 0 || i >= m_dataset->samples.size())
        return;
    const auto &s = m_dataset->samples[i];
    const auto &a = m_threshold->currentIndex() == 0   ? m_experiment.lower[i]
                    : m_threshold->currentIndex() == 2 ? m_experiment.upper[i]
                                                       : m_experiment.baseline[i];
    QString text =
        QStringLiteral("含义：当前证据下的利用信号优先级；不是综合风险评分、预测置信度或安全结论。\n缺失值不"
                       "等于零；KEV NotListed 不证明不存在利用；依赖路径不证明运行时可达。\n\n");
    text += s.id + "\n" + s.profile.key.snapshotIdentity.ecosystem + " / " +
            s.profile.key.snapshotIdentity.name + " @ " + s.profile.key.snapshotIdentity.version;
    text += "\nRepresentative OSV: " + s.candidate.id() + "\nAlias cluster SHA-256: " + s.clusterHash +
            "\nCluster IDs (deduplication only): " + s.clusterIds.join(", ");
    text += "\nPrimary stratum: " + s.stratum + "\n\n" + priorityExplanation(a) + "\n\n" +
            RiskEvidence::profileText(s.profile);
    m_detail->setPlainText(text);
}
