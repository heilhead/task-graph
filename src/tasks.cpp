#include "tasks.h"

TaskGraph& tasks::getGraph() {
    return TaskGraph::get();
}

void tasks::init(uint32_t numThreads) {
    TaskGraph::init(numThreads);
}

void tasks::shutdown() {
    TaskGraph::shutdown();
}

void tasks::wait(Task& task) {
    TaskGraph::get().getThreadWorker()->wait(&task);
}
