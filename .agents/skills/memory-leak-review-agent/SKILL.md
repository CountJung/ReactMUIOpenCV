---
name: memory-leak-review-agent
description: Use when Codex suspects leaked resources or missing cleanup in React, TypeScript, WebView2 hosting, C++ ownership, OpenCV processing, threads, sockets, file handles, object URLs, timers, subscriptions, or async work.
---

# Memory Leak Review Agent

Use this sub-agent when the main agent suspects memory, resource, lifecycle, or ownership leaks in React/TypeScript or C++ code.

## Trigger

- React effects that create timers, subscriptions, observers, workers, object URLs, sockets, or in-flight async work without cleanup.
- Components retaining large image/video blobs, previews, or base64 payloads longer than needed.
- C++ code that allocates memory or acquires resources without clear RAII ownership and release.
- Threads, jobs, callbacks, sockets, file handles, OpenCV resources, or WebView2 objects with unclear shutdown paths.

## React Checklist

- `useEffect` cleanup releases timers, event listeners, observers, WebSocket handlers, object URLs, and abortable requests.
- Large media buffers and previews are released or replaced rather than accumulated.
- Query subscriptions and cache updates avoid retaining stale closures or unbounded data.
- Component unmount paths do not leave background work running.

## C++ Checklist

- Prefer RAII types, smart pointers, standard containers, and deterministic destructors.
- Match any unavoidable `new/delete`, `malloc/free`, or API-specific acquire/release pairs.
- Ensure worker threads, job queues, sockets, file handles, and WebView2 resources have explicit shutdown ownership.
- Avoid reference cycles and dangling callbacks into destroyed services.
- Treat OpenCV `cv::Mat` and video/image resources as owned values or scoped handles with clear lifetime.

## Output

- Fix narrow leaks directly when safe.
- Otherwise report the suspected leak path, owning object, missing release point, and recommended ownership model.
