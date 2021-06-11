// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <QAbstractItemModel>
#include <QDockWidget>
#include <QTreeView>

#include "common/common_types.h"
#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/svc_common.h"

class EmuThread;

namespace Kernel {
class KHandleTable;
class KReadableEvent;
class KSynchronizationObject;
class KThread;
} // namespace Kernel

class WaitTreeThread;

class WaitTreeItem : public QObject {
    Q_OBJECT
public:
    WaitTreeItem();
    ~WaitTreeItem() override;

    virtual bool IsExpandable() const;
    virtual std::vector<std::unique_ptr<WaitTreeItem>> GetChildren() const;
    virtual QString GetText() const = 0;
    virtual QColor GetColor() const;

    void Expand();
    WaitTreeItem* Parent() const;
    const std::vector<std::unique_ptr<WaitTreeItem>>& Children() const;
    std::size_t Row() const;
    static std::vector<std::unique_ptr<WaitTreeThread>> MakeThreadItemList();

private:
    std::size_t row;
    bool expanded = false;
    WaitTreeItem* parent = nullptr;
    std::vector<std::unique_ptr<WaitTreeItem>> children;
};

class WaitTreeText : public WaitTreeItem {
    Q_OBJECT
public:
    explicit WaitTreeText(QString text);
    ~WaitTreeText() override;

    QString GetText() const override;

private:
    QString text;
};

class WaitTreeExpandableItem : public WaitTreeItem {
    Q_OBJECT
public:
    WaitTreeExpandableItem();
    ~WaitTreeExpandableItem() override;

    bool IsExpandable() const override;
};

class WaitTreeMutexInfo : public WaitTreeExpandableItem {
    Q_OBJECT
public:
    explicit WaitTreeMutexInfo(VAddr mutex_address, const Kernel::KHandleTable& handle_table);
    ~WaitTreeMutexInfo() override;

    QString GetText() const override;
    std::vector<std::unique_ptr<WaitTreeItem>> GetChildren() const override;

private:
    VAddr mutex_address{};
    u32 mutex_value{};
    Kernel::Handle owner_handle{};
    Kernel::KThread* owner{};
};

class WaitTreeCallstack : public WaitTreeExpandableItem {
    Q_OBJECT
public:
    explicit WaitTreeCallstack(const Kernel::KThread& thread);
    ~WaitTreeCallstack() override;

    QString GetText() const override;
    std::vector<std::unique_ptr<WaitTreeItem>> GetChildren() const override;

private:
    const Kernel::KThread& thread;
};

class WaitTreeSynchronizationObject : public WaitTreeExpandableItem {
    Q_OBJECT
public:
    explicit WaitTreeSynchronizationObject(const Kernel::KSynchronizationObject& object);
    ~WaitTreeSynchronizationObject() override;

    static std::unique_ptr<WaitTreeSynchronizationObject> make(
        const Kernel::KSynchronizationObject& object);
    QString GetText() const override;
    std::vector<std::unique_ptr<WaitTreeItem>> GetChildren() const override;

protected:
    const Kernel::KSynchronizationObject& object;
};

class WaitTreeObjectList : public WaitTreeExpandableItem {
    Q_OBJECT
public:
    WaitTreeObjectList(const std::vector<Kernel::KSynchronizationObject*>& list, bool wait_all);
    ~WaitTreeObjectList() override;

    QString GetText() const override;
    std::vector<std::unique_ptr<WaitTreeItem>> GetChildren() const override;

private:
    const std::vector<Kernel::KSynchronizationObject*>& object_list;
    bool wait_all;
};

class WaitTreeThread : public WaitTreeSynchronizationObject {
    Q_OBJECT
public:
    explicit WaitTreeThread(const Kernel::KThread& thread);
    ~WaitTreeThread() override;

    QString GetText() const override;
    QColor GetColor() const override;
    std::vector<std::unique_ptr<WaitTreeItem>> GetChildren() const override;
};

class WaitTreeEvent : public WaitTreeSynchronizationObject {
    Q_OBJECT
public:
    explicit WaitTreeEvent(const Kernel::KReadableEvent& object);
    ~WaitTreeEvent() override;
};

class WaitTreeThreadList : public WaitTreeExpandableItem {
    Q_OBJECT
public:
    explicit WaitTreeThreadList(std::vector<Kernel::KThread*>&& list);
    ~WaitTreeThreadList() override;

    QString GetText() const override;
    std::vector<std::unique_ptr<WaitTreeItem>> GetChildren() const override;

private:
    std::vector<Kernel::KThread*> thread_list;
};

class WaitTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit WaitTreeModel(QObject* parent = nullptr);
    ~WaitTreeModel() override;

    QVariant data(const QModelIndex& index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;

    void ClearItems();
    void InitItems();

private:
    std::vector<std::unique_ptr<WaitTreeThread>> thread_items;
};

class WaitTreeWidget : public QDockWidget {
    Q_OBJECT

public:
    explicit WaitTreeWidget(QWidget* parent = nullptr);
    ~WaitTreeWidget() override;

public slots:
    void OnDebugModeEntered();
    void OnDebugModeLeft();

    void OnEmulationStarting(EmuThread* emu_thread);
    void OnEmulationStopping();

private:
    QTreeView* view;
    WaitTreeModel* model;
};
