$VERSION="v0.1-alpha"
$DST_DIR="build"
$DST_FILE="$DST_DIR/builder.hpp"
$DST_SHA256="b177bf1d22ac38418f856c5be665c443ba12d153ba3385456c9b0593339c3c59" # Leave empty to ignore 

If (-Not (Test-Path $DST_DIR)) {
	New-Item -ItemType directory -Path $DST_DIR
}

If (-Not (Test-Path $DST_FILE)) {
	[Net.ServicePointManager]::SecurityProtocol = "Tls12, Tls11, Tls, Ssl3"
	Invoke-WebRequest -Uri https://github.com/mohaque0/cradle/releases/download/$VERSION/builder.hpp -OutFile $DST_FILE
}

$actualHash = $(CertUtil -hashfile build\builder.hpp SHA256)[1]
If ([string]::IsNullOrEmpty($DST_SHA256)) {
	Write-Output "Not checking hash because expected hash is empty."
} ELSE {
	Write-Output "Checking cradle hash."
	If (-Not (($actualHash) -Eq ($DST_SHA256))) {
		Write-Output "Downloaded file hash $actualHash doesn't match the expected $DST_SHA256."
		return
	}
}

iex "cl /I$DST_DIR build.cpp /std:c++14 /Fo:$DST_DIR/build.obj /Fe:$DST_DIR/cradle"
iex "$DST_DIR\cradle $args"