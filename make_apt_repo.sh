#!/usr/bin/env bash
# ============================================================
# Timeline Videos — static APT repository builder (GitHub Pages)
#
# Produces ./deb_repo/ — a flat, apt-installable repository:
#   *.deb   Packages / Packages.gz   Release   [Release.gpg  InRelease]
#
# Typical flow:
#   1. ./build_deb.sh                       # (called automatically)
#   2. GPG_KEY=<your key id> ./make_apt_repo.sh    # signing optional
#   3. Push deb_repo/ contents to the "gh-pages" branch
#      (or any static host).
#   4. On the target machine:
#        curl -fsSL <host>/key.gpg | sudo gpg --dearmor \
#            -o /usr/share/keyrings/timeline-videos.gpg
#        echo "deb [signed-by=/usr/share/keyrings/timeline-videos.gpg] \
#            <host>/ ./" | sudo tee /etc/apt/sources.list.d/timeline-videos.list
#        sudo apt update && sudo apt install timeline-videos
#
# Without GPG_KEY the repo is built unsigned — apt still accepts it
# with  deb [trusted=yes] <host> ./  (a warning is shown).
# ============================================================
set -e
cd "$(dirname "$0")"

REPO_DIR="deb_repo"
HOST_HINT="https://RAGAEIDOS.github.io/TimelineVideos"

# ---------- 1. make sure the .deb exists & is fresh ----------
./build_deb.sh

# ---------- 2. stage ----------
rm -rf "$REPO_DIR"
mkdir -p "$REPO_DIR"
cp dist/deb/*.deb "$REPO_DIR/"

# ---------- 3. package indexes ----------
cd "$REPO_DIR"
dpkg-scanpackages --multiversion . /dev/null > Packages
gzip -9kf Packages
echo "→ Packages / Packages.gz written"

# ---------- 4. Release (+ signatures when a key is given) ----------
apt-ftparchive release . > Release
if [ -n "${GPG_KEY:-}" ]; then
    gpg --batch -u "$GPG_KEY" --clearsign --digest-algo SHA256 \
        -o InRelease Release
    gpg --batch -u "$GPG_KEY" --detach-sign --armor --digest-algo SHA256 \
        -o Release.gpg Release
    # export the public key for users (they curl this file)
    gpg --batch -u "$GPG_KEY" --export --armor > key.gpg
    echo "→ Signed with key: $GPG_KEY  (+ key.gpg exported)"
else
    echo "→ UNSIGNED (set GPG_KEY=<keyid> to sign next time)"
fi

cd ..
echo
echo "Done → ${REPO_DIR}/"
echo "Contents:"
ls -1 "$REPO_DIR"
echo
echo "Publish: push ${REPO_DIR}/ contents to the gh-pages branch, e.g.:"
echo "  git checkout --orphan gh-pages"
echo "  git rm -rf . && cp -r ${REPO_DIR}/* ."
echo "  git add . && git commit -m 'APT repo' && git push origin gh-pages"
echo
echo "Users then install with:"
echo "  echo 'deb [trusted=yes] ${HOST_HINT}/ ./' | \\"
echo "      sudo tee /etc/apt/sources.list.d/timeline-videos.list"
echo "  sudo apt update && sudo apt install timeline-videos"
