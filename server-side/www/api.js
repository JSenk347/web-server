const express = require("express");
const app = express();

// import file system (fs) and path modules
const fs = require("fs");
const path = require("path");


// --- Painting Handler Functions ---

const getAllPaintings = (req, resp) => {
    const fileName = "paintings-nested.json";

    const jsonPath = path.join(__dirname, "data", fileName); // make path to the file described by fileName

    // read the file
    fs.readFile(jsonPath, (err, contents) => {
        err ? resp.status(500).send("Error reading the data file.") : resp.json(JSON.parse(contents));
    })
}

const getPaintingById = (req, resp) => {
    const fileName = "paintings-nested.json";
    const jsonPath = path.join(__dirname, "data", fileName);

    fs.readFile(jsonPath, (err, contents) => {
        if (err){
            resp.status(500).send("Error reading data file.")
        } else {
            const paintings = JSON.parse(contents);
            const matches = paintings.find(p => p.paintingID == req.params.id);

            if (matches){
                resp.json(matches);
            } else {
                resp.status(404).json({message: "Painting not found :("})
            }
        }
    });
}

const getPaintingsByGalleryId = (req, resp) => {
    const fileName = "paintings-nested.json";
    const jsonPath = path.join(__dirname, "data", fileName);

    fs.readFile(jsonPath, (err, contents) => {
        if (err){
            resp.status(500).send("Error reading data file.");
        } else {
            const paintings = JSON.parse(contents);
            const matches = paintings.filter(p => p.gallery.galleryID == req.params.id);

            if (matches.length > 0){
                resp.json(matches);
            } else {
                resp.status(404).json({message: "No paintings found in this gallery :("})
            }
        }
    });
}

const getPaintingsByArtistId = (req, resp) => {
    const fileName = "paintings-nested.json";
    const jsonPath = path.join(__dirname, "data", fileName);

    fs.readFile(jsonPath, (err, contents) => {
        if (err){
            resp.status(500).send("Error reading data file.")
        } else {
            const paintings = JSON.parse(contents);
            const matches = paintings.filter(p => p.artist.artistID == req.params.id);

            if (matches.length > 0){
                resp.json(matches);
            } else {
                resp.status(404).json({message: "No paintings by this artist have been found :("});
            }
        }
    });
}

const getPaintingsByYearRange = (req, resp) => {
    const fileName = "paintings-nested.json"
    const jsonPath = path.join(__dirname, "data", fileName);

    fs.readFile(jsonPath, (err, content) => {
        if (err){
            resp.status(500).send("Error reading data file.")
        } else {
            const paintings = JSON.parse(content);
            const matches = paintings.filter(p => p.yearOfWork <= req.params.max && p.yearOfWork >= req.params.min)

            if (matches.length > 0){
                resp.json(matches);
            } else {
                resp.status(404).json({message: `No paintings made in the range ${resp.params.min} - ${resp.params.max}`});
            }
        }
    });
}

// --- Artist Handler Functions ---



// --- Route Registration ---
app.get("/api/paintings", getAllPaintings);
app.get("/api/paintings/:id", getPaintingById); // :id means that "id is a parameter". eg "invocation" http://localhost:3000/api/paintings/441
app.get("/api/paintings/gallery/:id", getPaintingsByGalleryId);
app.get("/api/paintings/artist/:id", getPaintingsByArtistId);
app.get("/api/paintings/year/:min/:max", getPaintingsByYearRange);

// --- Server Spin-up ---
const port = process.env.PORT || 3000;
app.listen(port, () => {
    console.log(`Server running on port ${port}`);
});