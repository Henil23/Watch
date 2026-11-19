//
// Created by Henil Patel  on 2026-10-16.
//
#include "ProcessEnumerator.h"

#include <libproc.h>
#include <sys/proc_info.h>
using namespace std;
vector<ProcessInfo> ProcessEnumerator::enumerate() const


{
 vector<ProcessInfo> processes;

 // First call with nullptr to get the amount of memory needed
 // to store all currently running process IDs.
 const int bufferSize = proc_listpids(
     PROC_ALL_PIDS,
     0,
     nullptr,
     0
 );

 if (bufferSize <= 0)
 {
  return processes;
 }

 // proc_listpids returns the required size in bytes.
 const int pidCount = bufferSize / sizeof(pid_t);

 vector<pid_t> pids(pidCount);

 const int bytesUsed = proc_listpids(
     PROC_ALL_PIDS,
     0,
     pids.data(),
     bufferSize
 );

 if (bytesUsed <= 0)
 {
  return processes;
 }

 const int actualPidCount = bytesUsed / sizeof(pid_t);

 for (int i = 0; i < actualPidCount; ++i)
 {
  const pid_t pid = pids[i];

  // Ignore invalid/empty PID entries.
  if (pid <= 0)
  {
   continue;
  }

  char nameBuffer[PROC_PIDPATHINFO_MAXSIZE] = {};

  // Ask macOS for the process name.
  const int nameLength = proc_name(
      pid,
      nameBuffer,
      sizeof(nameBuffer)
  );

  if (nameLength <= 0)
  {
   continue;
  }

  ProcessInfo info;
  info.pid = pid;
  info.name = nameBuffer;
  // Retrieve basic task information for the process from macOS.
  proc_taskinfo taskInfo {};

  const int bytesReturned = proc_pidinfo(
      pid,
      PROC_PIDTASKINFO,
      0,
      &taskInfo,
      sizeof(taskInfo)
  );

  // Some processes may restrict access to their information.
  if (bytesReturned == sizeof(taskInfo))
  {
   // Collect memory and cumulative CPU time in one API call.
   info.residentMemoryBytes = taskInfo.pti_resident_size;

   info.cpuTimeTicks =
       taskInfo.pti_total_user +
       taskInfo.pti_total_system;

   info.hasTaskInfo = true;
  }

  processes.push_back(info);
 }

 return processes;
}