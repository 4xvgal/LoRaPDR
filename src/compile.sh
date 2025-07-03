#!/bin/bash

# 디렉토리 설정
SRC_DIR="./src"
BIN_DIR="./bin"

# bin 디렉토리가 없다면 생성
mkdir -p "$BIN_DIR"

# 빌드 대상 파일
SENDER_SRC="$SRC_DIR/sender.c"
RECEIVER_SRC="$SRC_DIR/receiver.c"

SENDER_BIN="$BIN_DIR/sender"
RECEIVER_BIN="$BIN_DIR/receiver"

# 컴파일
echo "🔨 Building sender..."
gcc "$SENDER_SRC" -o "$SENDER_BIN" || { echo "❌ Failed to build sender"; exit 1; }

echo "🔨 Building receiver..."
gcc "$RECEIVER_SRC" -o "$RECEIVER_BIN" || { echo "❌ Failed to build receiver"; exit 1; }

echo "✅ Build complete!"