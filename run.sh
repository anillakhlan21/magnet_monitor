#!/usr/bin/env bash
set -euo pipefail

# Simple run wrapper for Magnet Monitor
# Usage: ./run.sh start|stop|status

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$ROOT_DIR/build/magnet_monitor"
LOG="$ROOT_DIR/magnet_monitor.log"
PIDFILE="$ROOT_DIR/magnet_monitor.pid"

start() {
  if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
    echo "magnet_monitor is already running (PID $(cat "$PIDFILE"))"
    exit 0
  fi

  if [ ! -x "$BIN" ]; then
    echo "Executable not found: $BIN"
    exit 1
  fi

  # rotate one previous log
  if [ -f "$LOG" ]; then
    mv "$LOG" "$LOG.1" || true
  fi

  echo "Starting magnet_monitor, logging to $LOG"
  cd "$ROOT_DIR"
  nohup "$BIN" > "$LOG" 2>&1 &
  echo $! > "$PIDFILE"
  sleep 1
  echo "Started (PID $(cat "$PIDFILE"))"
}

stop() {
  if [ ! -f "$PIDFILE" ]; then
    echo "No PID file found, is the service running?"
    exit 0
  fi
  PID=$(cat "$PIDFILE")
  echo "Stopping magnet_monitor (PID $PID)"
  kill "$PID" && rm -f "$PIDFILE"
}

status() {
  if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
    echo "magnet_monitor running (PID $(cat "$PIDFILE"))"
  else
    echo "magnet_monitor not running"
  fi
}

case ${1-} in
  start) start ;; 
  stop) stop ;; 
  status) status ;; 
  *) echo "Usage: $0 start|stop|status"; exit 2 ;;
esac
