# Tesseract OCR Language Data

Place Tesseract `.traineddata` files in this folder when using Contour Extractor OCR.

Common files:

- `eng.traineddata` for English
- `kor.traineddata` for Korean

The backend also checks `TESSDATA_PREFIX` and common vcpkg tessdata locations. If both
`eng.traineddata` and `kor.traineddata` are present, the Contour Extractor defaults to
the `eng+kor` language combination.

Large `.traineddata` files are ignored by Git.
