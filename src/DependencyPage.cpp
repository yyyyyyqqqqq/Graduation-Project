#include "DependencyPage.h"

#include <QAbstractTableModel>
#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QSplitter>
#include <QTableView>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QtConcurrentRun>

namespace {
QString display(const QString& text) { return text.size()>512 ? text.left(512)+QStringLiteral("…") : text; }
QString stateText(ReferenceState state)
{
    switch(state) {
    case ReferenceState::Missing: return QStringLiteral("Missing / 缺失");
    case ReferenceState::Unknown: return QStringLiteral("Unknown / 未找到");
    case ReferenceState::Resolved: return QStringLiteral("Resolved / 唯一解析");
    case ReferenceState::Ambiguous: return QStringLiteral("Ambiguous / 多个匹配");
    }
    Q_UNREACHABLE();
}
QLabel* label(const QString& name,QWidget* parent)
{
    auto* value=new QLabel(parent);
    value->setObjectName(name);
    value->setTextFormat(Qt::PlainText);
    value->setWordWrap(true);
    return value;
}
}

// All large tables are views over the same immutable graph, with no per-cell
// allocation. The only separate row collection is the current on-demand query.
class DependencyTableModel final : public QAbstractTableModel
{
public:
    enum class Kind { Entries,Targets,Components,Relationships };
    DependencyTableModel(Kind kind,QObject* parent):QAbstractTableModel(parent),m_kind(kind) {}
    int rowCount(const QModelIndex& parent={}) const override {
        if(parent.isValid() || !m_graph) return 0;
        switch(m_kind) {
        case Kind::Entries: return int(m_graph->snapshot().entries.size());
        case Kind::Targets: return m_entry>=0 ? int(m_graph->snapshot().entries.at(m_entry).targets.size()) : 0;
        case Kind::Components: return int(m_graph->snapshot().components.size());
        case Kind::Relationships: return int(m_rows.size());
        }
        return 0;
    }
    int columnCount(const QModelIndex& parent={}) const override { return parent.isValid()?0:headers().size(); }
    QVariant headerData(int section,Qt::Orientation direction,int role) const override {
        if(direction==Qt::Horizontal && role==Qt::DisplayRole) return headers().value(section);
        return QAbstractTableModel::headerData(section,direction,role);
    }
    QVariant data(const QModelIndex& index,int role) const override {
        if(!index.isValid() || index.row()<0 || index.row()>=rowCount() || role!=Qt::DisplayRole) return {};
        const int row=index.row(),col=index.column();
        const auto& s=m_graph->snapshot();
        if(m_kind==Kind::Entries) {
            const auto& e=s.entries.at(row);
            switch(col) {
            case 0:return e.sourceOrder+1;
            case 1:return display(e.sourceRef);
            case 2:return stateText(m_graph->resolutions().at(row).source.state);
            case 3:return e.targets.size();
            }
        } else if(m_kind==Kind::Targets) {
            const auto& target=s.entries.at(m_entry).targets.at(row);
            const auto resolution=m_graph->resolutions().at(m_entry).targets.at(row);
            switch(col) {
            case 0:return target.targetOrder+1;
            case 1:return display(target.targetRef);
            case 2:return stateText(resolution.state);
            case 3:return resolution.state==ReferenceState::Resolved ? componentName(resolution.component) : QStringLiteral("—");
            }
        } else {
            const int node=m_kind==Kind::Components?row:m_rows.at(row).component;
            const auto& c=s.components.at(node);
            switch(col) {
            case 0:return componentName(node);
            case 1:return display(c.bomRef);
            case 2:return c.sourceRole==ComponentSourceRole::MetadataRoot ? QStringLiteral("元数据根组件") : QStringLiteral("组件 #%1").arg(c.sourceOrder+1);
            case 3:return m_transitive ? QString::number(m_rows.at(row).depth) : QStringLiteral("—");
            case 4:return node==m_origin ? QStringLiteral("Self dependency / 自依赖") : QString();
            }
        }
        return {};
    }
    void setGraph(std::shared_ptr<const DependencyGraph> graph) {
        beginResetModel(); m_graph=std::move(graph); m_entry=-1; m_rows.clear(); endResetModel();
    }
    void setEntry(int entry) { beginResetModel(); m_entry=entry; endResetModel(); }
    void setRows(QList<DependencyReach> rows,int origin,bool transitive) {
        beginResetModel(); m_rows=std::move(rows); m_origin=origin; m_transitive=transitive; endResetModel();
    }
private:
    QString componentName(int node) const {
        const auto& c=m_graph->snapshot().components.at(node);
        const auto position=c.sourceRole==ComponentSourceRole::MetadataRoot ? QStringLiteral("根组件") : QStringLiteral("#%1").arg(c.sourceOrder+1);
        return QStringLiteral("%1 · %2").arg(position,c.name.isEmpty()?QStringLiteral("（名称缺失）"):display(c.name));
    }
    QStringList headers() const {
        switch(m_kind) {
        case Kind::Entries:return {QStringLiteral("Entry #"),QStringLiteral("Source Ref"),QStringLiteral("Source Resolution"),QStringLiteral("Target Count")};
        case Kind::Targets:return {QStringLiteral("Target #"),QStringLiteral("Target Ref"),QStringLiteral("Target Resolution"),QStringLiteral("Resolved Component")};
        case Kind::Components:return {QStringLiteral("当前组件 / Name"),QStringLiteral("bom-ref"),QStringLiteral("来源")};
        case Kind::Relationships:return {QStringLiteral("关联组件 / Name"),QStringLiteral("bom-ref"),QStringLiteral("来源"),QStringLiteral("Shortest Depth"),QStringLiteral("说明")};
        }
        return {};
    }
    Kind m_kind;
    std::shared_ptr<const DependencyGraph> m_graph;
    int m_entry=-1, m_origin=-1;
    bool m_transitive=false;
    QList<DependencyReach> m_rows;
};

DependencyPage::DependencyPage(QString databasePath,QWidget* parent)
    :QWidget(parent),m_databasePath(std::move(databasePath))
{
    setObjectName(QStringLiteral("dependencyPage"));
    auto* outer=new QVBoxLayout(this);
    outer->setContentsMargins(0,0,0,0);
    auto* scroll=new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);
    auto* content=new QWidget(scroll);
    scroll->setWidget(content);
    auto* layout=new QVBoxLayout(content);
    layout->setContentsMargins(4,4,4,4);
    auto* actions=new QHBoxLayout;
    m_status=label(QStringLiteral("dependencyStatus"),this);
    actions->addWidget(m_status,1);
    m_reload=new QPushButton(QStringLiteral("重新分析 / Reload"),this);
    m_reload->setObjectName(QStringLiteral("reloadDependencies"));
    actions->addWidget(m_reload);
    layout->addLayout(actions);
    auto* tabs=new QTabWidget(this);
    tabs->setObjectName(QStringLiteral("dependencyTabs"));
    // At the application's minimum size, scroll the inspection area instead of
    // squeezing two table viewports down to their headers.
    connect(tabs,&QTabWidget::currentChanged,content,[content](int index){content->setMinimumHeight(index>0?360:0);});
    layout->addWidget(tabs,1);
    m_summary=new QTextBrowser(tabs);
    m_summary->setObjectName(QStringLiteral("dependencySummary"));
    m_summary->setOpenLinks(false);
    tabs->addTab(m_summary,QStringLiteral("摘要"));
    const auto table=[&](QWidget* parent,const QString& name,DependencyTableModel::Kind kind,DependencyTableModel*& model) {
        auto* view=new QTableView(parent);
        view->setObjectName(name);
        model=new DependencyTableModel(kind,view);
        view->setModel(model);
        view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setAlternatingRowColors(true);
        view->setWordWrap(false);
        view->setMinimumSize(0,0);
        view->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Ignored);
        view->horizontalHeader()->setDefaultSectionSize(200);
        return view;
    };
    auto* raw=new QSplitter(Qt::Vertical,tabs);
    m_entries=table(raw,QStringLiteral("dependencyEntries"),DependencyTableModel::Kind::Entries,m_entryModel);
    m_targets=table(raw,QStringLiteral("dependencyTargets"),DependencyTableModel::Kind::Targets,m_targetModel);
    raw->addWidget(m_entries); raw->addWidget(m_targets);
    raw->setChildrenCollapsible(false);
    tabs->addTab(raw,QStringLiteral("原始声明（上方选择 Entry）"));
    auto* relations=new QWidget(tabs);
    auto* relationshipLayout=new QVBoxLayout(relations);
    relationshipLayout->setContentsMargins(4,4,4,4);
    m_direction=new QComboBox(relations);
    m_direction->setObjectName(QStringLiteral("dependencyDirection"));
    m_direction->addItems({QStringLiteral("Direct Dependencies / 直接依赖"),QStringLiteral("Direct Dependents / 直接被依赖"),
        QStringLiteral("Transitive Dependencies / 全部可达依赖（含直接）"),QStringLiteral("Transitive Dependents / 全部可达被依赖（含直接）")});
    relationshipLayout->addWidget(m_direction);
    auto* split=new QSplitter(Qt::Vertical,relations);
    m_components=table(split,QStringLiteral("dependencyComponents"),DependencyTableModel::Kind::Components,m_componentModel);
    m_relationships=table(split,QStringLiteral("dependencyRelationships"),DependencyTableModel::Kind::Relationships,m_relationshipModel);
    split->addWidget(m_components); split->addWidget(m_relationships); split->setChildrenCollapsible(false);
    relationshipLayout->addWidget(split,1);
    m_relationshipStatus=label(QStringLiteral("dependencyRelationshipStatus"),relations);
    relationshipLayout->addWidget(m_relationshipStatus);
    tabs->addTab(relations,QStringLiteral("组件关系（上方选择组件）"));
    connect(m_reload,&QPushButton::clicked,this,&DependencyPage::reload);
    connect(m_entries->selectionModel(),&QItemSelectionModel::currentRowChanged,this,[this](const QModelIndex& i){ selectEntry(i.row()); });
    connect(m_components->selectionModel(),&QItemSelectionModel::currentRowChanged,this,[this]{ requestRelationship(); });
    connect(m_direction,&QComboBox::currentIndexChanged,this,[this]{ requestRelationship(); });
    connect(&m_analysis,&QFutureWatcher<AnalysisResult>::finished,this,[this] {
        const auto result=m_analysis.result();
        m_loading=false;
        if(m_activeGeneration==m_generation && m_activeProject==m_projectId) {
            if(result.result.ok()) installGraph(result.graph);
            else { m_status->setText(QStringLiteral("分析失败")); m_summary->setPlainText(result.result.userMessage()); }
            emit analysisFinished(result.result.ok());
        }
        // Coalesce rapid invalidations: only the latest request is started next.
        if(m_pendingAnalysis) startAnalysis();
    });
    connect(&m_query,&QFutureWatcher<QList<DependencyReach>>::finished,this,[this] {
        const auto rows=m_query.result();
        m_querying=false;
        if(m_activeQuery==m_queryGeneration && m_graph) {
            m_relationshipModel->setRows(rows,m_components->currentIndex().row(),m_direction->currentIndex()>=2);
            m_relationshipStatus->setText(QStringLiteral("可靠关系结果：%1；未解析声明不进入图，空结果不代表没有依赖。").arg(rows.size()));
            emit relationshipFinished();
        }
        if(m_pendingQuery) startRelationship();
    });
    invalidate();
}

void DependencyPage::invalidate()
{
    ++m_generation;
    ++m_queryGeneration;
    m_pendingAnalysis=false;
    m_pendingQuery=false;
    m_graph.reset();
    for(auto* model:{m_entryModel,m_targetModel,m_componentModel,m_relationshipModel}) model->setGraph({});
    m_relationshipStatus->setText(QStringLiteral("选择当前组件查看可靠关系。"));
    m_summary->setPlainText(QStringLiteral("尚无当前分析结果。依赖分析只基于已应用的当前 SBOM。"));
    m_status->setText(m_projectId.isEmpty()?QStringLiteral("请选择项目。"):QStringLiteral("待分析"));
    m_reload->setEnabled(!m_projectId.isEmpty());
}

void DependencyPage::setProject(const QString& projectId)
{
    m_projectId=projectId;
    invalidate();
    if(!m_projectId.isEmpty() && isVisible()) { m_pendingAnalysis=true; startAnalysis(); }
}

void DependencyPage::reload()
{
    invalidate();
    if(!m_projectId.isEmpty()) {m_pendingAnalysis=true; startAnalysis();}
}

void DependencyPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if(!m_projectId.isEmpty() && !m_graph && (!m_loading || m_activeGeneration!=m_generation || m_activeProject!=m_projectId)) {
        m_pendingAnalysis=true;
        startAnalysis();
    }
}

void DependencyPage::startAnalysis()
{
    if(m_loading || !m_pendingAnalysis || m_projectId.isEmpty()) return;
    m_pendingAnalysis=false;
    m_loading=true;
    m_activeGeneration=m_generation;
    m_activeProject=m_projectId;
    m_status->setText(QStringLiteral("正在读取并分析依赖…"));
    m_analysis.setFuture(QtConcurrent::run([path=m_databasePath,id=m_projectId] {
        DependencySnapshot snapshot;
        const auto result=ComponentRepository::readSnapshotInFile(path,id,snapshot);
        if(!result.ok()) return AnalysisResult{result,{}};
        return AnalysisResult{result,std::make_shared<const DependencyGraph>(DependencyGraph::build(std::move(snapshot)))};
    }));
}

void DependencyPage::installGraph(std::shared_ptr<const DependencyGraph> graph)
{
    m_graph=std::move(graph);
    const auto& snapshot=m_graph->snapshot();
    if(!snapshot.captured) {
        m_status->setText(QStringLiteral("Not Captured / 尚未捕获"));
        m_summary->setPlainText(snapshot.components.isEmpty()
            ? QStringLiteral("依赖数据尚未捕获，请导入并应用 SBOM。")
            : QStringLiteral("当前组件存在，但依赖数据尚未捕获，请重新导入并应用 SBOM。"));
        return; // Never present historical absence as zero dependencies.
    }
    m_status->setText(QStringLiteral("Captured / 已捕获"));
    const auto& m=m_graph->metrics();
    m_summary->setPlainText(QStringLiteral("%1\n\n原始声明 / Raw Metrics\nDependency Entries：%2\nDeclared Targets：%3\n"
        "Missing Reference Occurrences：%4\nUnknown Reference Occurrences：%5\nAmbiguous Reference Occurrences：%6\nSelf-Dependency Occurrences：%7\n\n"
        "派生可靠图 / Derived Graph Metrics\nResolved Unique Edges：%8\n\n"
        "引用只按原始 bom-ref 精确解析；Missing / Unknown / Ambiguous 保留在原始声明中，不生成可靠图边。无法可靠解析不等于没有依赖。\n"
        "Transitive 包含直接关系，按最短深度与首次发现排序，始终排除起点自身。超长字段仅在表格显示时省略。")
        .arg(snapshot.entries.isEmpty()?QStringLiteral("当前已应用的 SBOM 未声明 dependency entries。"):QStringLiteral("当前已应用的 SBOM 依赖分析。"))
        .arg(m.entries).arg(m.targets).arg(m.missing).arg(m.unknown).arg(m.ambiguous).arg(m.selfOccurrences).arg(m.uniqueEdges));
    for(auto* model:{m_entryModel,m_targetModel,m_componentModel,m_relationshipModel}) model->setGraph(m_graph);
}

void DependencyPage::selectEntry(int row)
{
    m_targetModel->setEntry(m_graph && row>=0 && row<m_graph->snapshot().entries.size()?row:-1);
}

void DependencyPage::requestRelationship()
{
    ++m_queryGeneration;
    m_pendingQuery=m_graph && m_graph->snapshot().captured && m_components->currentIndex().isValid();
    m_relationshipModel->setRows({},-1,false);
    m_relationshipStatus->setText(m_pendingQuery?QStringLiteral("正在查询所选组件…"):QStringLiteral("选择当前组件查看可靠关系。"));
    startRelationship();
}

void DependencyPage::startRelationship()
{
    if(m_querying || !m_pendingQuery || !m_graph) return;
    m_pendingQuery=false;
    m_querying=true;
    m_activeQuery=m_queryGeneration;
    const int origin=m_components->currentIndex().row(),direction=m_direction->currentIndex();
    m_query.setFuture(QtConcurrent::run([graph=m_graph,origin,direction] {
        if(direction>=2) return graph->transitive(origin,direction==3);
        QList<DependencyReach> rows;
        for(const int node:graph->direct(origin,direction==1)) rows.append({node,1});
        return rows;
    }));
}
