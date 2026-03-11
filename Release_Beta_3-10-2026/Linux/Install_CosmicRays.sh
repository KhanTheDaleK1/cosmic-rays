#!/bin/bash
echo "Installing Cosmic Rays (Linux)..."
mkdir -p ~/.vst3 ~/.lv2
cp -r "Cosmic Rays.vst3" ~/.vst3/ 2>/dev/null
cp -r "Cosmic Rays.lv2" ~/.lv2/ 2>/dev/null
echo "Installation complete!"
sleep 2