# Maintainer: trahloc <github@trahloc.com>
pkgname=autowaybar-ai
pkgver=1.0.0
pkgrel=1
pkgdesc="AI-enhanced version of autowaybar - program that hides waybar based on cursor position for Hyprland"
arch=('x86_64')
url="https://github.com/Trahloc/autowaybar-ai"
license=('MIT')
depends=('waybar' 'hyprland' 'fmt' 'jsoncpp')
makedepends=('xmake' 'gcc')
provides=('autowaybar')
conflicts=('autowaybar')
source=("$pkgname-$pkgver.tar.gz::https://github.com/Trahloc/autowaybar-ai/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$pkgname-$pkgver"
    xmake f -m release
    xmake -P .
}

package() {
    cd "$pkgname-$pkgver"
    
    # Install binary
    install -Dm755 build/linux/x86_64/release/autowaybar "$pkgdir/usr/bin/autowaybar"
    
    # Install license
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
    
    # Install README
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
}
